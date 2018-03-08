//
// Copyright (C) 2014 Jens Korinth, TU Darmstadt
//
// This file is part of Tapasco (TAPASCO).
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
/**
 *  @file	tapasco_status.h
 *  @brief	Thin wrapper around platform_status.
 *  @author	J. Korinth, TU Darmstadt (jk@esa.cs.tu-darmstadt.de)
 **/
#ifndef TAPASCO_STATUS_H__
#define TAPASCO_STATUS_H__

#include <tapasco_types.h>
#include <tapasco_global.h>
#include <platform.h>
#include <platform_caps.h>
#include <platform_status.h>

#define TAPASCO_VERSION_MAJOR(v) 				((v) >> 16)
#define TAPASCO_VERSION_MINOR(v) 				((v) & 0xFFFF)

typedef platform_status_t tapasco_status_t;
typedef platform_capabilities_0_t tapasco_capabilities_0_t;

tapasco_res_t tapasco_status_init(tapasco_status_t **status);
void tapasco_status_deinit(tapasco_status_t *status);

inline
int tapasco_status_has_capability_0(const tapasco_status_t *status,
		tapasco_capabilities_0_t caps)
{
	return platform_status_has_capability_0(status, caps);
}

inline
uint32_t tapasco_status_get_vivado_version(const tapasco_status_t *status)
{
	return platform_status_get_vivado_version(status);
}

inline
uint32_t tapasco_status_get_tapasco_version(const tapasco_status_t *status)
{
	return platform_status_get_tapasco_version(status);
}

inline
uint32_t tapasco_status_get_gen_ts(const tapasco_status_t *status)
{
	return platform_status_get_gen_ts(status);
}

inline
uint32_t tapasco_status_get_host_clk(const tapasco_status_t *status)
{
	return platform_status_get_host_clk(status);
}

inline
uint32_t tapasco_status_get_mem_clk(const tapasco_status_t *status)
{
	return platform_status_get_mem_clk(status);
}

inline
uint32_t tapasco_status_get_design_clk(const tapasco_status_t *status)
{
	return platform_status_get_design_clk(status);
}

inline
tapasco_handle_t tapasco_status_get_slot_base(const tapasco_status_t *status,
		const tapasco_slot_id_t slot_id)
{
	return platform_status_get_slot_base(status, slot_id);
}

#endif /* TAPASCO_STATUS_H__ */
