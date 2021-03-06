//
// Copyright (C) 2017 Jens Korinth, TU Darmstadt
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
package de.tu_darmstadt.cs.esa.tapasco.itapasco.controller
import  de.tu_darmstadt.cs.esa.tapasco.itapasco.view.selection._
import  de.tu_darmstadt.cs.esa.tapasco.itapasco.view.detail._

/** Selection-Detail panel controller for [[base.Architecture]].
 *  Controls instances of [[view.selection.ArchitecturesPanel]] and
 *  [[view.detail.ArchitectureDetailPanel]].
 *  @constructor Create new instance of controller.
 */
class ArchitecturesPanelController extends {
  val architectures = new ArchitecturesPanel
  val details   = new ArchitectureDetailPanel
} with SelectionDetailViewController(ViewController(architectures), ViewController(details)) {
  // details view should listen to selection view
  architectures.Selection += details
}
