plugins {
    application
    kotlin("jvm") version "2.1.10"
}

repositories {
    mavenCentral()
}

dependencies {
    implementation(project(":library"))
}

application {
    mainClass.set("io.tira.measure.app.AppKt")
}

kotlin {
    jvmToolchain(17)
}