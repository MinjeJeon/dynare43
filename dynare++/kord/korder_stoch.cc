// Copyright 2005, Ondra Kamenik

#include "korder_stoch.hh"

/* Same as |@<|MatrixA| constructor code@>|, but the submatrix |gss_ys| is
   passed directly. */

MatrixAA::MatrixAA(const FSSparseTensor &f, const IntSequence &ss,
                   const TwoDMatrix &gss_ys, const PartitionY &ypart)
  : PLUMatrix(ypart.ny())
{
  zeros();

  IntSequence c(1); c[0] = 1;
  FGSTensor f_y(f, ss, c, TensorDimens(ss, c));
  add(1.0, f_y);

  c[0] = 0;
  FGSTensor f_yss(f, ss, c, TensorDimens(ss, c));
  TwoDMatrix sub(*this, ypart.nstat, ypart.nys());
  sub.multAndAdd(f_yss, gss_ys);

  calcPLU();
}

// |KOrderStoch| folded constructor code
KOrderStoch::KOrderStoch(const PartitionY &yp, int nu,
                         const TensorContainer<FSSparseTensor> &fcont,
                         const FGSContainer &hh, Journal &jr)
  : nvs{yp.nys(), nu, nu, 1}, ypart(yp), journal(jr),
    _ug(4), _fg(4), _ugs(4), _fgs(4), _uG(4), _fG(4),
    _uh(nullptr), _fh(&hh),
    _uZstack(&_uG, ypart.nyss(), &_ug, ypart.ny(), ypart.nys(), nu),
    _fZstack(&_fG, ypart.nyss(), &_fg, ypart.ny(), ypart.nys(), nu),
    _uGstack(&_ugs, ypart.nys(), nu),
    _fGstack(&_fgs, ypart.nys(), nu),
    f(fcont),
    matA(*(fcont.get(Symmetry{1})), _uZstack.getStackSizes(), *(hh.get(Symmetry{1, 0, 0, 0})),
         ypart)
{
}

// |KOrderStoch| unfolded constructor code
KOrderStoch::KOrderStoch(const PartitionY &yp, int nu,
                         const TensorContainer<FSSparseTensor> &fcont,
                         const UGSContainer &hh, Journal &jr)
  : nvs{yp.nys(), nu, nu, 1}, ypart(yp), journal(jr),
    _ug(4), _fg(4), _ugs(4), _fgs(4), _uG(4), _fG(4),
    _uh(&hh), _fh(nullptr),
    _uZstack(&_uG, ypart.nyss(), &_ug, ypart.ny(), ypart.nys(), nu),
    _fZstack(&_fG, ypart.nyss(), &_fg, ypart.ny(), ypart.nys(), nu),
    _uGstack(&_ugs, ypart.nys(), nu),
    _fGstack(&_fgs, ypart.nys(), nu),
    f(fcont),
    matA(*(fcont.get(Symmetry{1})), _uZstack.getStackSizes(), *(hh.get(Symmetry{1, 0, 0, 0})),
         ypart)
{
}

// |KOrderStoch| convenience method specializations
template<>
ctraits<KOrder::unfold>::Tg& KOrderStoch::g<KOrder::unfold>()
{
  return _ug;
}
template<>
const ctraits<KOrder::unfold>::Tg& KOrderStoch::g<KOrder::unfold>() const
{ return _ug;}
template<>
ctraits<KOrder::fold>::Tg& KOrderStoch::g<KOrder::fold>()
{
  return _fg;
}
template<>
const ctraits<KOrder::fold>::Tg& KOrderStoch::g<KOrder::fold>() const
{ return _fg;}
template<>
ctraits<KOrder::unfold>::Tgs& KOrderStoch::gs<KOrder::unfold>()
{
  return _ugs;
}
template<>
const ctraits<KOrder::unfold>::Tgs& KOrderStoch::gs<KOrder::unfold>() const
{ return _ugs;}
template<>
ctraits<KOrder::fold>::Tgs& KOrderStoch::gs<KOrder::fold>()
{
  return _fgs;
}
template<>
const ctraits<KOrder::fold>::Tgs& KOrderStoch::gs<KOrder::fold>() const
{ return _fgs;}
template<>
const ctraits<KOrder::unfold>::Tgss& KOrderStoch::h<KOrder::unfold>() const
{ return *_uh;}
template<>
const ctraits<KOrder::fold>::Tgss& KOrderStoch::h<KOrder::fold>() const
{ return *_fh;}
template<>
ctraits<KOrder::unfold>::TG& KOrderStoch::G<KOrder::unfold>()
{
  return _uG;
}
template<>
const ctraits<KOrder::unfold>::TG& KOrderStoch::G<KOrder::unfold>() const
{ return _uG;}
template<>
ctraits<KOrder::fold>::TG& KOrderStoch::G<KOrder::fold>()
{
  return _fG;
}
template<>
const ctraits<KOrder::fold>::TG& KOrderStoch::G<KOrder::fold>() const
{ return _fG;}
template<>
ctraits<KOrder::unfold>::TZXstack& KOrderStoch::Zstack<KOrder::unfold>()
{
  return _uZstack;
}
template<>
const ctraits<KOrder::unfold>::TZXstack& KOrderStoch::Zstack<KOrder::unfold>() const
{ return _uZstack;}
template<>
ctraits<KOrder::fold>::TZXstack& KOrderStoch::Zstack<KOrder::fold>()
{
  return _fZstack;
}
template<>
const ctraits<KOrder::fold>::TZXstack& KOrderStoch::Zstack<KOrder::fold>() const
{ return _fZstack;}
template<>
ctraits<KOrder::unfold>::TGXstack& KOrderStoch::Gstack<KOrder::unfold>()
{
  return _uGstack;
}
template<>
const ctraits<KOrder::unfold>::TGXstack& KOrderStoch::Gstack<KOrder::unfold>() const
{ return _uGstack;}
template<>
ctraits<KOrder::fold>::TGXstack& KOrderStoch::Gstack<KOrder::fold>()
{
  return _fGstack;
}
template<>
const ctraits<KOrder::fold>::TGXstack& KOrderStoch::Gstack<KOrder::fold>() const
{ return _fGstack;}
