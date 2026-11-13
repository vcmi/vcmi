#!/usr/bin/env python

from conan.api.conan_api import ConanAPI
from conan.api.model import ListPattern
from datetime import datetime
from glob import glob
import sys

api = ConanAPI()
# relies on private API
if len(sys.argv) > 1:
    api._api_helpers.set_core_confs([f"core.cache:storage_path={sys.argv[1]}"])

def packageDate(package):
    return datetime.fromtimestamp(package.timestamp).replace(microsecond=0)

ndk = api.list.select(ListPattern("android-ndk/*:*"))
qt = api.list.select(ListPattern("qt/*:*"), package_query="os=Android")

print("\nNDK list:")
for recipe, packagesDic in ndk.items():
    for package in packagesDic:
        ndkPath = api.cache.package_path(package)
        compilerInfo = ""
        androidVersionPath = glob(f"{ndkPath}/bin/toolchains/llvm/prebuilt/*/AndroidVersion.txt")
        if androidVersionPath:
            with open(androidVersionPath[0], "r", encoding="utf-8") as f:
                compilerInfo = f" - compiler v{f.readline().rstrip()}"
        print(f"{recipe.version}{compilerInfo} - {packageDate(package)}")
        print(f"\t{ndkPath}")

print("\nQt list:")
qtSerialized = qt.serialize()
for recipe, packagesDic in qt.items():
    for package in packagesDic:
        packageSettings = qtSerialized[f"{recipe.name}/{recipe.version}"]["revisions"][recipe.revision]["packages"][package.package_id]["info"]["settings"]
        print(f"{recipe.version} for {packageSettings["arch"]}, compiler v{packageSettings["compiler.version"]} - {packageDate(package)}")
        print(f"\t{api.cache.package_path(package)}")
