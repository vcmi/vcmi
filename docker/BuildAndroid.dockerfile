FROM ubuntu:noble
WORKDIR /usr/local/app

RUN apt-get update && apt-get install -y openjdk-17-jdk python3 pipx cmake ccache ninja-build wget git xz-utils

ENV PIPX_HOME="/opt/pipx"
ENV PIPX_BIN_DIR="/usr/local/bin"
ENV PIPX_MAN_DIR="/usr/local/share/man"
RUN pipx install 'conan<2.0'
RUN pipx install 'sdkmanager==0.6.10'

RUN conan profile new conan --detect

RUN wget https://github.com/vcmi/vcmi-dependencies/releases/download/1.3/dependencies-android-arm64-v8a.txz
RUN tar -xf dependencies-android-arm64-v8a.txz -C ~/.conan
RUN rm dependencies-android-arm64-v8a.txz

ENV JAVA_HOME="/usr/lib/jvm/java-17-openjdk-amd64"
ENV ANDROID_HOME="/usr/lib/android-sdk"
ENV GRADLE_USER_HOME="/vcmi/.cache/grandle"
ENV GENERATE_ONLY_BUILT_CONFIG=1

RUN sdkmanager --install "platform-tools"
RUN sdkmanager --install "platforms;android-34"
RUN yes | sdkmanager --licenses

RUN conan download android-ndk/r25c@:4db1be536558d833e52e862fd84d64d75c2b3656 -r conancenter

CMD ["sh", "-c", " \
    # switch to mounted dir
    cd /vcmi ; \
    # install conan stuff
    conan install . --install-folder=conan-generated --no-imports --build=never --profile:build=default --profile:host=CI/conan/android-64-ndk ; \
    # link conan ndk that grandle can find it
    mkdir -p /usr/lib/android-sdk/ndk ; \
    ln -s -T ~/.conan/data/android-ndk/r25c/_/_/package/4db1be536558d833e52e862fd84d64d75c2b3656/bin /usr/lib/android-sdk/ndk/25.2.9519653 ; \
    # build
    cmake --preset android-daily-release ; \
    cmake --build --preset android-daily-release \
"]
