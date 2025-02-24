plugins {
    application
    kotlin("jvm")
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