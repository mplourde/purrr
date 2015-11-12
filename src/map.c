#include <R.h>
#include <Rinternals.h>

// call must involve i
SEXP call_loop(SEXP env, SEXP call, int n, SEXPTYPE type) {
  // Create variable "i" and map to scalar integer
  SEXP i_val = PROTECT(Rf_ScalarInteger(1));
  SEXP i = Rf_install("i");
  Rf_defineVar(i, i_val, env);
  UNPROTECT(1);

  SEXP out = PROTECT(Rf_allocVector(type, n));
  for (int i = 0; i < n; ++i) {
    INTEGER(i_val)[0] = i + 1;

    SEXP res = Rf_eval(call, env);
    if (type != VECSXP && (Rf_length(res) != 1 || TYPEOF(res) != type))
      Rf_error("Result %i is not a length 1 %s", i + 1, Rf_type2char(type));

    switch(type) {
    case LGLSXP:
    case INTSXP:  INTEGER(out)[i] = INTEGER(res)[0]; break;
    case REALSXP: REAL(out)[i] = REAL(res)[0]; break;
    case STRSXP:  SET_STRING_ELT(out, i, STRING_ELT(res, 0)); break;
    case VECSXP:  SET_VECTOR_ELT(out, i, res); break;
    default:      Rf_error("Unsupported type %s", Rf_type2char(type));
    }
  }
  UNPROTECT(1);

  return out;
}

SEXP map_impl(SEXP env, SEXP x_name_, SEXP f_name_, SEXP type_) {
  const char* x_name = CHAR(asChar(x_name_));
  const char* f_name = CHAR(asChar(f_name_));

  SEXP x = Rf_install(x_name);
  SEXP f = Rf_install(f_name);
  SEXP i = Rf_install("i");

  SEXP x_val = Rf_eval(x, env);
  if (!Rf_isVector(x_val))
    Rf_error("`.x` is not a vector (%s)", Rf_type2char(TYPEOF(x_val)));
  int n = Rf_length(x_val);

  // Constructs a call like f(x[[i]], ...) - don't want to substitute
  // actual values for f or x, because they may be long, which creates
  // bad tracebacks()
  SEXP Xi = PROTECT(Rf_lang3(R_Bracket2Symbol, x, i));
  SEXP f_call = PROTECT(Rf_lang3(f, Xi, R_DotsSymbol));

  SEXPTYPE type = Rf_str2type(CHAR(Rf_asChar(type_)));
  SEXP out = PROTECT(call_loop(env, f_call, n, type));

  SEXP names = Rf_getAttrib(x_val, R_NamesSymbol);
  if(!Rf_isNull(names))
    Rf_setAttrib(out, R_NamesSymbol, names);

  UNPROTECT(3);

  return out;
}

SEXP map2_impl(SEXP env, SEXP x_name_, SEXP y_name_, SEXP f_name_, SEXP type_) {
  const char* x_name = CHAR(Rf_asChar(x_name_));
  const char* y_name = CHAR(Rf_asChar(y_name_));
  const char* f_name = CHAR(Rf_asChar(f_name_));

  SEXP x = Rf_install(x_name);
  SEXP y = Rf_install(y_name);
  SEXP f = Rf_install(f_name);
  SEXP i = Rf_install("i");

  SEXP x_val = Rf_eval(x, env);
  if (!Rf_isVector(x_val))
    Rf_error("`.x` is not a vector (%s)", Rf_type2char(TYPEOF(x_val)));
  SEXP y_val = Rf_eval(y, env);
  if (!Rf_isVector(y_val))
    Rf_error("`.y` is not a vector (%s)", Rf_type2char(TYPEOF(y_val)));

  int nx = Rf_length(x_val), ny = Rf_length(y_val);
  if (nx != ny && !(nx == 1 || ny == 1)) {
    Rf_error("`.x` (%i) and `.y` (%i) are different lengths", nx, ny);
  }
  int n = (nx > ny) ? nx : ny;

  // Constructs a call like f(x[[i]], y[[i]], ...)
  SEXP one = PROTECT(Rf_ScalarInteger(1));
  SEXP Xi = PROTECT(Rf_lang3(R_Bracket2Symbol, x, nx == 1 ? one : i));
  SEXP Yi = PROTECT(Rf_lang3(R_Bracket2Symbol, y, ny == 1 ? one : i));
  SEXP f_call = PROTECT(Rf_lang4(f, Xi, Yi, R_DotsSymbol));

  SEXPTYPE type = Rf_str2type(CHAR(Rf_asChar(type_)));
  SEXP out = PROTECT(call_loop(env, f_call, n, type));

  SEXP names = Rf_getAttrib(x_val, R_NamesSymbol);
  if(!Rf_isNull(names))
    Rf_setAttrib(out, R_NamesSymbol, names);

  UNPROTECT(5);
  return out;
}
