#include "Var6.d"
#include "__oo2c.h"

static void Var6__Set(OOC_INT16 y) {
  register OOC_INT32 i0;

  i0 = y;
  Var6__x = i0;
  return;
  ;
}

static OOC_INT16 Var6__F(OOC_CHAR8 b) {
  register OOC_INT32 i0;

  i0 = b;
  if (i0) goto l3;
  Var6__Set(0);
  goto l4;
l3:
  Var6__x = 1;
l4:
  i0 = Var6__x;
  return i0;
  ;
}

void Var6__Test() {
  register OOC_INT32 i0;

  Var6__x = -1;
  i0 = Var6__F(OOC_FALSE);
  Out0__Int(i0, 0);
  Out0__Ln();
  i0 = Var6__x;
  Out0__Int(i0, 0);
  Out0__Ln();
  i0 = Var6__F(OOC_TRUE);
  Out0__Int(i0, 0);
  Out0__Ln();
  i0 = Var6__x;
  Out0__Int(i0, 0);
  Out0__Ln();
  return;
  ;
}

void OOC_Var6_init(void) {

  return;
  ;
}

/* --- */
