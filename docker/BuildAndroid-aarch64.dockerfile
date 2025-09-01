FROM ubuntu:noble
WORKDIR /usr/local/app

RUN apt-get update && apt-get install -y openjdk-17-jdk python3 pipx cmake ccache ninja-build wget git xz-utils

ENV PIPX_HOME="/opt/pipx"
ENV PIPX_BIN_DIR="/usr/local/bin"
ENV PIPX_MAN_DIR="/usr/local/share/man"
RUN pipx install 'conan'
RUN pipx install 'sdkmanager'

RUN conan profile detect

ENV DEPS_VERSION="2025-08-24"
ENV DEPS="dependencies-android-arm64-v8a.tgz"
RUN wget --https-only --max-redirect=20 https://github.com/vcmi/vcmi-dependencies/releases/download/$DEPS_VERSION/$DEPS
RUN conan cache restore $DEPS
RUN rm $DEPS

ENV JAVA_HOME="/usr/lib/jvm/java-17-openjdk-amd64"
ENV ANDROID_HOME="/usr/lib/android-sdk"
ENV GRADLE_USER_HOME="/vcmi/.cache/gradle"
ENV NDK_VERSION="25.2.9519653"

RUN sdkmanager --install "platform-tools" "platforms;android-35" "ndk;$NDK_VERSION"
RUN yes | sdkmanager --licenses

CMD ["sh", "-c", " \
    # switch to mounted dir
    cd /vcmi ; \
    # prepare Gradle config
    mkdir -p $GRADLE_USER_HOME ; \
    echo android.bundle.enableUncompressedNativeLibs=true > $GRADLE_USER_HOME/gradle.properties ; \
    # generate CMake toolchain
    conan install . --output-folder=conan-generated --build=never --profile=dependencies/conan_profiles/android-64 --profile=dependencies/conan_profiles/base/android-system -c tools.android:ndk_path=$ANDROID_HOME/ndk/$NDK_VERSION && \
    # build
    cmake --preset android-daily-release && \
    cmake --build --preset android-daily-release \
"]
