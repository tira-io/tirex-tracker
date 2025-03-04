import groovy.lang.Closure
import org.jetbrains.kotlin.gradle.tasks.KotlinCompile

plugins {
    `java-library`
    `maven-publish`
    signing
    kotlin("jvm")
    kotlin("plugin.serialization")
    id("com.palantir.git-version")
    id("com.github.gmazzo.buildconfig")
}

val gitVersion: Closure<String> by extra

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

val javaLanguageVersion = JavaLanguageVersion.of(8)
val javaLanguageVersionTest = JavaLanguageVersion.of(17)

kotlin {
    jvmToolchain {
        languageVersion = javaLanguageVersion
    }
}

java {
    withSourcesJar()
    withJavadocJar()
}

tasks {
    test {
        javaLauncher = project.javaToolchains.launcherFor {
            languageVersion = javaLanguageVersionTest
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

    withType<KotlinCompile>().configureEach {
//        dependsOn(generateBuildConstants)
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
        register<MavenPublication>("gpr") {
            groupId = "io.tira"
            artifactId = "tirex-tracker"
            version = gitVersion()

            from(components["java"])

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

        register<MavenPublication>("mavenCentral") {
            groupId = "io.tira"
            artifactId = "tirex-tracker"
            version = gitVersion()

            from(components["java"])

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
    }
}

//signing {
//    val signingKey: String? by project
//    val signingPassword: String? by project
//    useInMemoryPgpKeys(signingKey, signingPassword)
//    sign(publishing.publications["mavenCentral"])
//}
