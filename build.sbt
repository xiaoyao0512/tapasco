name := "chisel-axiutils"

organization := "esa.cs.tu-darmstadt.de"

version := "0.4-SNAPSHOT"

scalaVersion := "2.11.11"

resolvers ++= Seq(
  Resolver.sonatypeRepo("snapshots"),
  Resolver.sonatypeRepo("releases")
)

// Provide a managed dependency on X if -DXVersion="" is supplied on the command line.
val defaultVersions = Map("chisel3"          -> "3.0-SNAPSHOT",
                          "chisel-iotesters" -> "1.1-SNAPSHOT")

libraryDependencies ++= (Seq("chisel3","chisel-iotesters").map {
  dep: String => "edu.berkeley.cs" %% dep % sys.props.getOrElse(dep + "Version", defaultVersions(dep)) })

libraryDependencies ++= Seq(
  "org.scalatest" %% "scalatest" % "3.0.4" % "test",
  "org.scalacheck" %% "scalacheck" % "1.13.5" % "test",
  "com.typesafe.play" %% "play-json" % "2.6.3"
)

scalacOptions ++= Seq("-language:implicitConversions", "-language:reflectiveCalls", "-deprecation", "-feature")

// project structure

lazy val packaging = project.in(file("packaging"))

lazy val miscutils = project.in(file("miscutils"))

lazy val root = (project in file(".")).dependsOn(packaging, miscutils, miscutils % "test->test")

cleanFiles += (baseDirectory.value / "test")

