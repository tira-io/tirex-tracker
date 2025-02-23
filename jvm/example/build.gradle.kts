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

kotlin {
    jvmToolchain(17)
}