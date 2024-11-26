pluginManagement {
    repositories {
        google {
            content {
                includeGroupByRegex("com\\.android.*")
                includeGroupByRegex("com\\.google.*")
                includeGroupByRegex("androidx.*")
            }
        }
        mavenCentral()
        gradlePluginPortal()
        maven {
            setUrl("https://qapm-maven.pkg.coding.net/repository/qapm_sdk/android_release/")
        }
    }
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
        maven {
            setUrl("https://maven.youzanyun.com/repository/maven-releases/")
        }
        maven {
            setUrl("https://qapm-maven.pkg.coding.net/repository/qapm_sdk/android_release/")
        }
    }
}

rootProject.name = "My Application"
include(":app")
include(":nativelib")
