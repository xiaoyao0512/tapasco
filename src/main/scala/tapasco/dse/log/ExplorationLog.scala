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
package de.tu_darmstadt.cs.esa.tapasco.dse.log
import  de.tu_darmstadt.cs.esa.tapasco.base._
import  de.tu_darmstadt.cs.esa.tapasco.dse._
import  de.tu_darmstadt.cs.esa.tapasco.dse.log.json._
import  de.tu_darmstadt.cs.esa.tapasco.util._
import  de.tu_darmstadt.cs.esa.tapasco.Logging._
import  scala.collection.mutable.ArrayBuffer
import  scala.io.Source
import  java.time.LocalDateTime
import  play.api.libs.json._

class ExplorationLog extends Listener[Exploration.Event] {
  protected val log: ArrayBuffer[ExplorationLog.Entry] = new ArrayBuffer
  def update(e: Exploration.Event): Unit = log.synchronized { log += ((LocalDateTime.now(), e)) }
  def events: Seq[ExplorationLog.Entry] = log.synchronized { log.toSeq }
}

object ExplorationLog extends Publisher {
  type Event = Exploration.Event
  type Entry = (LocalDateTime, Exploration.Event)
  private implicit val logger = de.tu_darmstadt.cs.esa.tapasco.Logging.logger(getClass)

  def apply(llog: Seq[Entry]): ExplorationLog = new ExplorationLog { this.log ++= llog }

  def replay(elog: ExplorationLog) {
    elog.log foreach { e => publish(e._2) }
  }

  def fromFile(filename: String): Option[(Configuration, ExplorationLog)] =
    catchAllDefault(None: Option[(Configuration, ExplorationLog)],
        "error while parsing ExplorationLog: ") {
      import de.tu_darmstadt.cs.esa.tapasco.base.json._
      val json = Json.parse(Source.fromFile(filename).getLines
        .mkString(scala.util.Properties.lineSeparator))
      Some(((json \ "Configuration").as[Configuration], (json \ "Events").as[ExplorationLog]))
  }

  def toFile(e: ExplorationLog, filename: String)(implicit cfg: Configuration): Unit = try {
    import de.tu_darmstadt.cs.esa.tapasco.base.json._
    val json = Json.prettyPrint(Json.obj(
      "Configuration" -> Json.toJson(cfg),
      "Events" -> Json.toJson(e)
    ))
    val fw = new java.io.FileWriter(filename)
    fw.write(json)
    fw.close()
  } catch { case e: Exception =>
    logger.warn("exception while writing ExplorationLog: {}", e)
  }
}
