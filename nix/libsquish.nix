{
  stdenv,
  fetchsvn,
  source ? null,
  version ? "1.15",
  cmake,
  git,
}: let
  _source =
    if source != null
    then source
    else
      fetchsvn {
        url = "svn://svn.code.sf.net/p/libsquish/code/trunk";
        rev = "r111";
        sha256 = "sha256-LMIpTIvFkGbBs3Ns2oVuIz3ulk/9DG0KMn8wNSJctEk=";
      };
in
  stdenv.mkDerivation {
    inherit version;
    pname = "libsquish";
    src = _source;
    buildInputs = [];
    nativeBuildInputs = [cmake git];
    doCheck = true;
    cmakeFlags = ["-DBUILD_SHARED_LIBS=ON"];
  }
