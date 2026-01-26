import groovy.lang.Closure
import org.gradle.api.tasks.testing.logging.TestExceptionFormat
import org.jreleaser.gradle.plugin.tasks.AbstractJReleaserTask
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
    id("org.jetbrains.dokka-javadoc")
    id("org.jreleaser")
}

val gitVersion: Closure<String> by extra

group = "io.tira"
version = gitVersion()

repositories {
    mavenCentral()
}

dependencies {
    implementation(kotlin("stdlib-jdk8"))
    implementation("net.java.dev.jna:jna-platform:5.18.1")
    api("net.java.dev.jna:jna:5.18.1")
    implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.10.0")
    implementation("org.yaml:snakeyaml:2.5")

    testImplementation("org.jetbrains.kotlin:kotlin-test")
    testImplementation("org.jetbrains.kotlin:kotlin-test-junit")
    testImplementation("org.junit.jupiter:junit-jupiter-engine:5.13.4")
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
            showStandardStreams = true
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
        dependsOn(dokkaGeneratePublicationHtml)
        from(dokkaGeneratePublicationHtml.flatMap { it.outputDirectory })
        archiveClassifier = "html-docs"
    }

    register<Jar>("javadocJar") {
        dependsOn(dokkaGeneratePublicationJavadoc)
        from(dokkaGeneratePublicationJavadoc.flatMap { it.outputDirectory })
        archiveClassifier = "javadoc"
    }

    withType<AbstractJReleaserTask> {
        dependsOn("publishAllPublicationsToJReleaserRepository")
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
                username = System.getenv("GITHUB_USERNAME")
                password = System.getenv("GITHUB_PASSWORD")
            }
        }

        maven {
            name = "JReleaser"
            url = layout.buildDirectory.dir("staging-deploy").get().asFile.toURI()
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
}

jreleaser {
    strict = true
    gitRootSearch = true

    project {
        name = "tirex-tracker"
        version = project.version.get()
        description = "Automatic resource and metadata tracking for information retrieval experiments."
        license = "MIT License"
        links {
            homepage = "https://github.com/tira-io/tirex-tracker"
            license = "https://opensource.org/license/MIT"
            bugTracker = "https://github.com/tira-io/tirex-tracker/issues"
            contact = "https://webis.de/people"
            vcsBrowser = "https://github.com/tira-io/tirex-tracker"
            contribute = "https://github.com/tira-io/tirex-tracker"
        }
        languages {
            java {
                groupId = "io.tirex"
                version = javaLanguageVersionCompile.asInt().toString()
            }
        }
        inceptionYear = "2025"
    }

    environment {
        variables = rootDir.resolve("jreleaser.yml")
    }

    signing {
        active = Active.ALWAYS
        armored = true
    }

    release {
        github {
            enabled = false
        }
    }

    deploy {
        maven {
            mavenCentral {
                register("sonatype") {
                    active = Active.ALWAYS
                    url = "https://central.sonatype.com/api/v1/publisher"
                    stagingRepository("build/staging-deploy")
                    applyMavenCentralRules = true

                    // Wait up to 60 * 30s = 30min for Maven Central to finalize the uploaded package.
                    maxRetries = 60
                    retryDelay = 30
                }
            }
        }
    }
}
