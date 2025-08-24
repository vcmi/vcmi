dic = {
    "applicationIdSuffix": ".daily",
    "applicationLabel": "VCMI daily",
    "applicationVariant": "daily",
    "signingConfig": "dailySigning",
}
print(";".join([f"{key}={value}" for key, value in dic.items()]))
