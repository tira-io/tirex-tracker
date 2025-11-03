plugins {
    kotlin("jvm") version "2.2.21" apply false
    kotlin("plugin.serialization") version "2.2.21" apply false
    id("com.palantir.git-version") version "3.3.0" apply false
    id("com.github.gmazzo.buildconfig") version "5.7.1" apply false
    id("org.jetbrains.dokka") version "2.1.0" apply false
    id("org.jreleaser") version "1.20.0" apply false
}
