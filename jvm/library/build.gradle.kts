import groovy.lang.Closure
import org.gradle.api.tasks.testing.logging.TestExceptionFormat
import org.jetbrains.kotlin.gradle.tasks.KotlinCompile
import org.jreleaser.model.Active

plugins {
    `java-library`
    `maven-publish`
    jacoco
    kotlin("jvm")
    kotlin("plugin.serialization")
    id("com.palantir.git-version")
    id("com.github.gmazzo.buildconfig")
    id("org.jetbrains.dokka")
    id("org.jreleaser")
}

val gitVersion: Closure<String> by extra

group = "io.tira"
version = gitVersion()

repositories {
    mavenCentral()
}

dependencies {
    implementation(platform("org.jetbrains.kotlin:kotlin-bom"))
    implementation("org.jetbrains.kotlin:kotlin-stdlib-jdk8")
    implementation("net.java.dev.jna:jna-platform:5.16.0")
    api("net.java.dev.jna:jna:5.16.0")
    implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.8.0")
    implementation("org.yaml:snakeyaml:2.4")

    testImplementation("org.jetbrains.kotlin:kotlin-test")
    testImplementation("org.jetbrains.kotlin:kotlin-test-junit")
    testImplementation("org.junit.jupiter:junit-jupiter-engine:5.12.0")
}

val javaLanguageVersionCompile = JavaLanguageVersion.of(8)

@Suppress("UnstableApiUsage")
val javaLanguageVersionTest = JavaLanguageVersion.current().also { maxOf(it, javaLanguageVersionCompile) }

kotlin {
    jvmToolchain {
        languageVersion = javaLanguageVersionCompile
    }
}

java {
    withSourcesJar()
}

jacoco {
    toolVersion = "0.8.10"
}

tasks {
    jacocoTestReport {
        reports {
            xml.required = true
            csv.required = true
            html.required = true
        }
    }

    test {
        javaLauncher = project.javaToolchains.launcherFor {
            languageVersion = javaLanguageVersionTest
        }

        finalizedBy(jacocoTestReport)

        testLogging {
            exceptionFormat = TestExceptionFormat.FULL
            events("passed", "failed", "skipped")
        }
    }

    compileTestKotlin {
        val launcher = project.javaToolchains.launcherFor {
            languageVersion = javaLanguageVersionTest
        }
        kotlinJavaToolchain.toolchain.use(launcher)
    }

    compileTestJava {
        javaCompiler = project.javaToolchains.compilerFor {
            languageVersion = javaLanguageVersionTest
        }
    }

    register<Jar>("htmlDocsJar") {
        dependsOn(dokkaHtml)
        from(dokkaHtml.flatMap { it.outputDirectory })
        archiveClassifier.set("html-docs")
    }

    register<Jar>("javadocJar") {
        dependsOn(dokkaJavadoc)
        from(dokkaJavadoc.flatMap { it.outputDirectory })
        archiveClassifier.set("javadoc")
    }
}

buildConfig {
    className("Build")   // forces the class name. Defaults to 'BuildConfig'
    packageName("io.tira.tirex.tracker")  // forces the package. Defaults to '${project.group}'
    buildConfigField("VERSION", provider(gitVersion))
}

publishing {
    repositories {
        maven {
            name = "GitHubPackages"
            url = uri("https://maven.pkg.github.com/tira-io/tirex-tracker")
            credentials {
                username = project.findProperty("gpr.user") as String? ?: System.getenv("USERNAME")
                password = project.findProperty("gpr.key") as String? ?: System.getenv("TOKEN")
            }
        }
    }

    publications {
        withType<MavenPublication> {
            groupId = project.group.toString()
            artifactId = "tirex-tracker"
            version = project.version.toString()

            from(components["java"])

            artifact(tasks["htmlDocsJar"])
            artifact(tasks["javadocJar"])

            pom {
                name = "tirex-tracker"
                description = "Automatic resource and metadata tracking for information retrieval experiments."
                url = "https://github.com/tira-io/tirex-tracker"
                licenses {
                    license {
                        name = "MIT License"
                        url = "https://opensource.org/license/MIT"
                    }
                }
                developers {
                    developer {
                        name = "Jan Heinrich Merker"
                        email = "heinrich.merker@uni-jena.de"
                    }
                    developer {
                        name = "Tim Hagen"
                        email = "tim.hagen@uni-kassel.de"
                    }
                    developer {
                        name = "Maik Fröbe"
                        email = "maik.froebe@uni-jena.de"
                    }
                }
                scm {
                    connection = "scm:git:git://github.com/tira-io/tirex-tracker.git"
                    developerConnection = "scm:git:ssh://git@github.com:tira-io/tirex-tracker.git"
                    url = "https://github.com/tira-io/tirex-tracker"
                }
            }
        }

        register<MavenPublication>("maven")
    }

    repositories {
        maven {
            url = layout.buildDirectory.dir("staging-deploy").get().asFile.toURI()
        }
    }
}

jreleaser {
    signing {
        active.set(Active.ALWAYS)
        armored = true
    }

    deploy {
        maven {
            mavenCentral {
                register("sonatype") {
                    active.set(Active.ALWAYS)
                    url.set("https://central.sonatype.com/api/v1/publisher")
                    stagingRepository("target/staging-deploy")
                }
            }
        }
    }
}
