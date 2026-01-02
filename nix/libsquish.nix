{
  stdenv,
  fetchsvn,
  source ?
    fetchsvn {
      url = "svn://svn.code.sf.net/p/libsquish/code/trunk";
      rev = "r111";
      sha256 = "sha256-LMIpTIvFkGbBs3Ns2oVuIz3ulk/9DG0KMn8wNSJctEk=";
    },
  version ? "1.15",
  cmake,
}:
stdenv.mkDerivation {
  inherit version;
  pname = "libsquish";
  src = source;
  buildInputs = [];
  nativeBuildInputs = [cmake];
  doCheck = true;
  cmakeFlags = ["-DBUILD_SHARED_LIBS=ON"];
}
