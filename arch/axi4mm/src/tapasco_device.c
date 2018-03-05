//
// Copyright (C) 2014 Jens Korinth, TU Darmstadt
//
// This file is part of Tapasco (TPC).
//
// Tapasco is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tapasco is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Tapasco.  If not, see <http://www.gnu.org/licenses/>.
//
//! @file	tapasco_device.c
//! @brief	Zynq Platform device struct and helper methods.
//!		Makes extensive use of the arch/common code snippets to
//!		implement the various TPC API calls.
//! @authors	J. Korinth, TU Darmstadt (jk@esa.cs.tu-darmstadt.de)
//!
//! @todo Remove stdio and debug output.
#include <stdio.h>
#include <tapasco_device.h>
#include <tapasco_jobs.h>
#include <tapasco_address_map.h>
#include <tapasco_scheduler.h>
#include <tapasco_logging.h>
#include <tapasco_status.h>
#include <tapasco_local_mem.h>
#include <platform.h>
#include <platform_errors.h>

/** Internal device struct implementation. */
struct tapasco_dev_ctx {
	tapasco_functions_t *functions;
	tapasco_jobs_t *jobs;
	tapasco_ctx_t *ctx;
	tapasco_dev_id_t id;
	tapasco_status_t *status;
	tapasco_local_mem_t *lmem;
};

tapasco_functions_t *tapasco_device_functions(tapasco_dev_ctx_t *ctx)
{
	return ctx->functions;
}

tapasco_status_t *tapasco_device_status(tapasco_dev_ctx_t *ctx)
{
	return ctx->status;
}

tapasco_local_mem_t *tapasco_device_local_mem(tapasco_dev_ctx_t *ctx)
{
	return ctx->lmem;
}

/** Interrupt handler callback. */
void irq_handler(int const event);

/** System setup function. */
static void setup_system(tapasco_dev_ctx_t *dev_ctx)
{
	// enable interrupts, globally and for each instance
	tapasco_functions_setup_system(dev_ctx, dev_ctx->functions);
}

/******************************************************************************/
/* TPC API implementation. */

tapasco_res_t tapasco_create_device(tapasco_ctx_t *ctx, tapasco_dev_id_t const dev_id,
		tapasco_dev_ctx_t **pdev_ctx,
		tapasco_device_create_flag_t const flags)
{
	tapasco_dev_ctx_t *p = (tapasco_dev_ctx_t *)malloc(sizeof(struct tapasco_dev_ctx));
	if (p) {
		tapasco_res_t res = tapasco_status_init(&p->status);
		res = res == TAPASCO_SUCCESS ? tapasco_functions_init(p->status,
				&p->functions) : res;
		res = res == TAPASCO_SUCCESS ? tapasco_jobs_init(&p->jobs) : res;
		res = res == TAPASCO_SUCCESS ? tapasco_local_mem_init(p->status, &p->lmem) : res;
		if (res != TAPASCO_SUCCESS) return res;
		p->ctx = ctx;
		p->id = dev_id;
		*pdev_ctx = p;
		setup_system(p);
		platform_register_irq_callback(irq_handler);
		LOG(LALL_DEVICE, "device %d created successfully", dev_id);
		return TAPASCO_SUCCESS;
	}
	return TAPASCO_FAILURE;
}

void tapasco_destroy_device(tapasco_ctx_t *ctx, tapasco_dev_ctx_t *dev_ctx)
{
	platform_stop(0);
	tapasco_local_mem_deinit(dev_ctx->lmem);
	tapasco_jobs_deinit(dev_ctx->jobs);
	tapasco_functions_deinit(dev_ctx->functions);
	tapasco_status_deinit(dev_ctx->status);
	free(dev_ctx);
}

uint32_t tapasco_device_func_instance_count(tapasco_dev_ctx_t *dev_ctx,
		tapasco_func_id_t const func_id)
{
	assert(dev_ctx);
	//! @todo Remove this when custom AXI regset IP core is available.
	return tapasco_functions_count(dev_ctx->functions, func_id);
}	

tapasco_res_t tapasco_device_load_bitstream_from_file(tapasco_dev_ctx_t *dev_ctx,
		char const *filename,
		tapasco_load_bitstream_flag_t const flags)
{
	return TAPASCO_ERR_NOT_IMPLEMENTED;
}

tapasco_res_t tapasco_device_load_bitstream(tapasco_dev_ctx_t *dev_ctx, size_t const len,
		void const *data,
		tapasco_load_bitstream_flag_t const flags)
{
	return TAPASCO_ERR_NOT_IMPLEMENTED;
}

tapasco_job_id_t tapasco_device_acquire_job_id(tapasco_dev_ctx_t *dev_ctx,
		tapasco_func_id_t const func_id,
		tapasco_device_acquire_job_id_flag_t flags)
{
	if (flags) return TAPASCO_ERR_NOT_IMPLEMENTED;
	tapasco_job_id_t j_id = tapasco_jobs_acquire(dev_ctx->jobs);
	if (j_id > 0) tapasco_jobs_set_func_id(dev_ctx->jobs, j_id, func_id);
	return j_id;
}

void tapasco_device_release_job_id(tapasco_dev_ctx_t *dev_ctx,
		tapasco_job_id_t const job_id)
{
	tapasco_jobs_release(dev_ctx->jobs, job_id);
}

tapasco_res_t tapasco_device_job_launch(tapasco_dev_ctx_t *dev_ctx,
		tapasco_job_id_t const j_id,
		tapasco_device_job_launch_flag_t const flags)
{
	if (flags & TAPASCO_DEVICE_JOB_LAUNCH_NONBLOCKING)
		return TAPASCO_ERR_NONBLOCKING_MODE_NOT_SUPPORTED;
	if (flags) return TAPASCO_ERR_NOT_IMPLEMENTED;
	return tapasco_scheduler_launch(dev_ctx, dev_ctx->jobs, dev_ctx->functions, j_id);
}

tapasco_res_t tapasco_device_job_get_arg(tapasco_dev_ctx_t *dev_ctx,
		tapasco_job_id_t const j_id, uint32_t arg_idx,
		size_t const arg_len, void *arg_value)
{
	return tapasco_jobs_get_arg(dev_ctx->jobs, j_id, arg_idx, arg_len, arg_value);
}

tapasco_res_t tapasco_device_job_set_arg(tapasco_dev_ctx_t *dev_ctx,
		tapasco_job_id_t const j_id, uint32_t arg_idx,
		size_t const arg_len, void const *arg_value)
{
	return tapasco_jobs_set_arg(dev_ctx->jobs, j_id, arg_idx, arg_len, arg_value);
}

tapasco_res_t tapasco_device_job_set_arg_transfer(tapasco_dev_ctx_t *dev_ctx,
		tapasco_job_id_t const job_id, uint32_t arg_idx,
		size_t const arg_len, void *arg_value,
		tapasco_device_alloc_flag_t const flags)
{
	return tapasco_jobs_set_arg_transfer(dev_ctx->jobs, job_id, arg_idx,
			arg_len, arg_value, flags);
}

tapasco_res_t tapasco_device_job_get_return(tapasco_dev_ctx_t *dev_ctx,
		tapasco_job_id_t const j_id, size_t const ret_len,
		void *ret_value)
{
	return tapasco_jobs_get_return(dev_ctx->jobs, j_id, ret_len, ret_value);
}

tapasco_res_t tapasco_device_has_capability(tapasco_dev_ctx_t *dev_ctx,
		tapasco_device_capability_t cap)
{
	switch (cap) {
	case TAPASCO_DEVICE_CAP_ATSPRI:
		return tapasco_status_has_capability_0(dev_ctx->status, TAPASCO_CAP0_ATSPRI);
	case TAPASCO_DEVICE_CAP_ATSCHECK:
		return tapasco_status_has_capability_0(dev_ctx->status, TAPASCO_CAP0_ATSCHECK);
	default:
		WRN("unknown capability: %d (0x%0x)", cap, cap);
		return TAPASCO_FAILURE;
	}
}

void irq_handler(int const event)
{
	LOG(LALL_IRQ, "IRQ event #%d", event);
}

