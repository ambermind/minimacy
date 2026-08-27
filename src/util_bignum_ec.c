// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"
#include"util_bignum.h"

void _x25519(bignum _P, bignum _Pprime, bignum _A24, bignum k, LINT t, bignum u, bignum _One, bignum* resultX, bignum* resultZ)
{
	bignum x1 = u;
	bignum x2 = bigCopy(_One);
	bignum x3 = bigCopy(u);
	bignum z2 = bigFromInt(0);
	bignum z3 = bigCopy(_One);
	LINT swap = 0;
	while (t >= 0) {
		LINT k_t = bigBit(k, t) ^ swap;
		bigKeepOrSwapConstantTime(k_t, &x2, &x3);
		bigKeepOrSwapConstantTime(k_t, &z2, &z3);
		//if (k_t) {
		//	bignum w;
		//	w = x2; x2 = x3; x3 = w;
		//	w = z2; z2 = z3; z3 = w;
		//}
		swap ^= k_t;

		//let x_2 + z_2->A in		// A  = x_2+z_2
		//let A * *2->AA in		// AA = A^2
		//let x_2 - z_2->B in		// B  = x_2-z_2
		//let B * *2->BB in		// BB = B^2
		//let AA - BB->E in		// E  = AA-BB
		bignum A = bigAddMod(x2, z2, _P);
		bignum AA = bigSquareModMontgomery(A, _P, _Pprime);
		bignum B = bigSubMod(x2, z2, _P);
		bignum BB = bigSquareModMontgomery(B, _P, _Pprime);
		bignum E= bigSubMod(AA, BB, _P);

		//let x_3 + z_3->C in		// C  = x_3+z_3
		//let x_3 - z_3->D in		// D  = x_3-z_3
		//let D * A->DA in		// DA = D*A
		//let C * B->CB in		// CB = C*B
		bignum C = bigAddMod(x3, z3, _P);
		bignum D = bigSubMod(x3, z3, _P);
		bignum DA = bigMulModMontgomery(D, A, _P, _Pprime);
		bignum CB = bigMulModMontgomery(C, B, _P, _Pprime);

		//let(DA + CB) * *2->x_3 in			// x_3=(DA+CB)^2
		x3 = bigReplace(x3, bigAddMod(DA, CB, _P));
		x3 = bigReplace(x3, bigSquareModMontgomery(x3, _P, _Pprime));
		//let x_1 * ((DA - CB) * *2)->z_3 in	// z_3 = x_1 * (DA - CB)^2
		z3 = bigReplace(z3, bigSubMod(DA, CB, _P));
		z3 = bigReplace(z3, bigSquareModMontgomery(z3, _P, _Pprime));
		z3 = bigReplace(z3, bigMulModMontgomery(x1, z3, _P, _Pprime));
		//let AA * BB->x_2 in					// x_2 = AA * BB
		x2= bigReplace(x2, bigMulModMontgomery(AA, BB, _P, _Pprime));
		//let E * (AA + _A24 * E)->z_2 in	// z_2 = E * (AA + a24 * E)
		z2 = bigReplace(z2, bigMulModMontgomery(_A24, E, _P, _Pprime));
		z2 = bigReplace(z2, bigAddMod(AA, z2, _P));
		z2 = bigReplace(z2, bigMulModMontgomery(E, z2, _P, _Pprime));

		bigRelease(A);
		bigRelease(AA);
		bigRelease(B);
		bigRelease(BB);
		bigRelease(E);
		bigRelease(C);
		bigRelease(D);
		bigRelease(DA);
		bigRelease(CB);
		t--;
	}
	*resultX = bigSelectOrRelease(swap, x2, x3);
	*resultZ = bigSelectOrRelease(swap, z2, z3);
}
bignum bigX25519(bignum _P, bignum _Pprime, bignum _Pminus2, bignum _A24, bignum _One, bignum k0, bignum u0)	// big alloc 19
{
	bignum x2, z2, u, k;
	LINT t = bigNbits(k0) - 1;
	if ((bigNbits(_P) > 256) || (bigNbits(_Pprime) > 256) || (bigNbits(_Pminus2) > 256) || (bigNbits(_A24) > 256) || (bigNbits(_One) > 256)) return NULL;

	u = bigMod(u0, _P);
	k = bigMod(k0, _P);

	u = bigReplace(u, bigMontgomeryForm(u, _P));
	_x25519(_P, _Pprime, _A24, k, t, u, _One, &x2, &z2);
	z2 = bigReplace(z2, bigExpModMontgomery(z2, _Pminus2, _P, _Pprime));
	z2 = bigReplace(z2, bigMulModMontgomery(x2, z2, _P, _Pprime));
	z2 = bigReplace(z2, bigMontgomeryRedc(z2, _P, _Pprime));
	bigRelease(x2);
	bigRelease(u);
	bigRelease(k);
	return z2;
}


void ED25519add(bignum _P, bignum _Pprime, bignum _D,
	bignum x1, bignum y1, bignum z1, bignum t1,
	bignum x2, bignum y2, bignum z2, bignum t2,
	bignum* x, bignum* y, bignum* z, bignum* t)
{
	//let(Y1 - X1)* (Y2 - X2)->A in
	//let(Y1 + X1)* (Y2 + X2)->B in
	//let T1* T2* _D->C in
	//let Z1* Z2->D in
	bignum A = bigSubMod(y1, x1, _P);
	bignum r = bigSubMod(y2, x2, _P);
	A = bigReplace(A, bigMulModMontgomery(A, r, _P, _Pprime));
	bignum B = bigAddMod(y1, x1, _P);
	r = bigReplace(r, bigAddMod(y2, x2, _P));
	B = bigReplace(B, bigMulModMontgomery(B, r, _P, _Pprime));
	bigRelease(r);
	bignum C = bigMulModMontgomery(t1, t2, _P, _Pprime);
	bignum D = bigMulModMontgomery(z1, z2, _P, _Pprime);
	C = bigReplace(C, bigMulModMontgomery(C, _D, _P, _Pprime));

	//let C + C->C in
	//let D + D->D in
	//let B - A->E in
	//let D - C->F in
	//let D + C->G in
	//let B + A->H in
	C = bigReplace(C, bigAddMod(C, C, _P));
	D = bigReplace(D, bigAddMod(D, D, _P));
	bignum E = bigSubMod(B, A, _P);
	bignum F = bigSubMod(D, C, _P);
	bignum G = bigAddMod(D, C, _P);
	bignum H = bigAddMod(B, A, _P);
	//[E * F, G * H, F * G, E * H];;
	*x = bigMulModMontgomery(E, F, _P, _Pprime);
	*y = bigMulModMontgomery(G, H, _P, _Pprime);
	*z = bigMulModMontgomery(F, G, _P, _Pprime);
	*t = bigMulModMontgomery(E, H, _P, _Pprime);
	bigRelease(A);
	bigRelease(B);
	bigRelease(C);
	bigRelease(D);
	bigRelease(E);
	bigRelease(F);
	bigRelease(G);
	bigRelease(H);
}
void ED25519double(bignum _P, bignum _Pprime, bignum _D, bignum* x, bignum* y, bignum* z, bignum* t)
{
	//let(Y1 - X1)** 2->A in
	//let(Y1 + X1) * *2->B in
	//let(T1 * *2) * _D->C in
	//let(Z1 * *2)->D in
	bignum A = bigSubMod(*y, *x, _P);
	bignum B = bigAddMod(*y, *x, _P);
	bignum C = bigSquareModMontgomery(*t, _P, _Pprime);
	bignum D = bigSquareModMontgomery(*z, _P, _Pprime);
	A = bigReplace(A, bigSquareModMontgomery(A, _P, _Pprime));
	B = bigReplace(B, bigSquareModMontgomery(B, _P, _Pprime));
	C = bigReplace(C, bigMulModMontgomery(C, _D, _P, _Pprime));
	//let C + C->C in
	//let D + D->D in
	//let B - A->E in
	//let D - C->F in
	//let D + C->G in
	//let B + A->H in
	C = bigReplace(C, bigAddMod(C, C, _P));
	D = bigReplace(D, bigAddMod(D, D, _P));
	bignum E= bigSubMod(B, A, _P);
	bignum F= bigSubMod(D, C, _P);
	bignum G= bigAddMod(D, C, _P);
	bignum H = bigAddMod(B, A, _P);
	//[E * F, G * H, F * G, E * H];;
	*x= bigReplace(*x, bigMulModMontgomery(E, F, _P, _Pprime));
	*y= bigReplace(*y, bigMulModMontgomery(G, H, _P, _Pprime));
	*z= bigReplace(*z, bigMulModMontgomery(F, G, _P, _Pprime));
	*t= bigReplace(*t, bigMulModMontgomery(E, H, _P, _Pprime));
	bigRelease(A);
	bigRelease(B);
	bigRelease(C);
	bigRelease(D);
	bigRelease(E);
	bigRelease(F);
	bigRelease(G);
	bigRelease(H);
}

int bigEd25519Add(bignum _P, bignum _Pprime, bignum _D, bignum x1, bignum y1, bignum z1, bignum t1,
	bignum x2, bignum y2, bignum z2, bignum t2,
	bignum* x, bignum* y, bignum* z, bignum* t)
{
	if ((bigNbits(_P) > 256) || (bigNbits(_Pprime) > 256) || (bigNbits(_D) > 256)) return -1;

	if (bigSign(x1) || bigSign(y1) || bigSign(z1) || bigSign(t1) || bigSign(x2) || bigSign(y2) || bigSign(z2) || bigSign(t2)) return -1;
	if (BIGNUM_GREATER_EQUAL(x1, _P)|| BIGNUM_GREATER_EQUAL(y1, _P) || BIGNUM_GREATER_EQUAL(z1, _P) || BIGNUM_GREATER_EQUAL(t1, _P)) return -1;
	if (BIGNUM_GREATER_EQUAL(x2, _P)|| BIGNUM_GREATER_EQUAL(y2, _P) || BIGNUM_GREATER_EQUAL(z2, _P) || BIGNUM_GREATER_EQUAL(t2, _P)) return -1;
	ED25519add(_P, _Pprime, _D,
		x1, y1, z1, t1,
		x2, y2, z2, t2,
		x, y, z, t);
	return 0;
}

int bigEd25519Mul(int constantTime, bignum _P, bignum _Pprime, bignum _D, bignum _One, bignum s, bignum px, bignum py, bignum pz, bignum pt, bignum* x, bignum* y, bignum* z, bignum* t)
{
	bignum qx = bigFromInt(0);
	bignum qy = bigCopy(_One);
	bignum qz = bigCopy(_One);
	bignum qt = bigFromInt(0);
	LINT nbits = bigNbits(s);
	LINT i;
	if ((bigNbits(_P) > 256) || (bigNbits(_Pprime) > 256) || (bigNbits(_D) > 256) || (bigNbits(_One) > 256)) return -1;

	px = bigMod(px, _P);
	py = bigMod(py, _P);
	pz = bigMod(pz, _P);
	pt = bigMod(pt, _P);
	if (constantTime) {
		for (i = 0; i < nbits; i++) {
			bignum tx, ty, tz, tt;
			LINT b = bigBit(s, i);
			ED25519add(_P, _Pprime, _D, px, py, pz, pt, qx, qy, qz, qt, &tx, &ty, &tz, &tt);
			qx = bigSelectOrRelease(b, qx, tx);
			qy = bigSelectOrRelease(b, qy, ty);
			qz = bigSelectOrRelease(b, qz, tz);
			qt = bigSelectOrRelease(b, qt, tt);
			ED25519double(_P, _Pprime, _D, &px, &py, &pz, &pt);
		}
	}
	else {
		for (i = 0; i < nbits; i++) {
			if (bigBit(s, i)) {
				bignum tx, ty, tz, tt;
				ED25519add(_P, _Pprime, _D, px, py, pz, pt, qx, qy, qz, qt, &tx, &ty, &tz, &tt);
				qx = bigReplace(qx, tx);
				qy = bigReplace(qy, ty);
				qz = bigReplace(qz, tz);
				qt = bigReplace(qt, tt);
			}
			ED25519double(_P, _Pprime, _D, &px, &py, &pz, &pt);
		}
	}
	bigRelease(px);
	bigRelease(py);
	bigRelease(pz);
	bigRelease(pt);
	*x = qx; *y = qy; *z = qz; *t = qt;
	return 0;
}


void weierstrassA0Add(bignum _P, bignum _Pprime, bignum b3,
	bignum X1, bignum Y1, bignum Z1,
	bignum X2, bignum Y2, bignum Z2,
	bignum* X, bignum* Y, bignum* Z)
{
	bignum t0, t1, t2, t3, t4, X3, Y3, Z3;
	if (BIGNUM_IS_NULL(Z1)) {
		*X = bigCopy(X2);
		*Y = bigCopy(Y2);
		*Z = bigCopy(Z2);
		return;
	}
	if (BIGNUM_IS_NULL(Z2)) {
		*X = bigCopy(X1);
		*Y = bigCopy(Y1);
		*Z = bigCopy(Z1);
		return;
	}

	t0 = bigMulModMontgomery(X1, X2, _P, _Pprime);
	t1 = bigMulModMontgomery(Y1, Y2, _P, _Pprime);
	t2 = bigMulModMontgomery(Z1, Z2, _P, _Pprime);

	t3 = bigAddMod(X1, Y1, _P);
	t4 = bigAddMod(X2, Y2, _P);
	t3 = bigReplace(t3, bigMulModMontgomery(t3, t4, _P, _Pprime));

	t4 = bigReplace(t4, bigAddMod(t0, t1, _P));
	t3 = bigReplace(t3, bigSubMod(t3, t4, _P));
	t4 = bigReplace(t4, bigAddMod(Y1, Z1, _P));

	X3 = bigAddMod(Y2, Z2, _P);
	t4 = bigReplace(t4, bigMulModMontgomery(t4, X3, _P, _Pprime));
	X3 = bigReplace(X3, bigAddMod(t1, t2, _P));

	t4 = bigReplace(t4, bigSubMod(t4, X3, _P));
	X3 = bigReplace(X3, bigAddMod(X1, Z1, _P));
	Y3 = bigAddMod(X2, Z2, _P);

	X3 = bigReplace(X3, bigMulModMontgomery(X3, Y3, _P, _Pprime));
	Y3 = bigReplace(Y3, bigAddMod(t0, t2, _P));
	Y3 = bigReplace(Y3, bigSubMod(X3, Y3, _P));

	X3 = bigReplace(X3, bigAddMod(t0, t0, _P));
	t0 = bigReplace(t0, bigAddMod(X3, t0, _P));
	t2 = bigReplace(t2, bigMulModMontgomery(b3, t2, _P, _Pprime));

	Z3 = bigAddMod(t1, t2, _P);
	t1 = bigReplace(t1, bigSubMod(t1, t2, _P));
	Y3 = bigReplace(Y3, bigMulModMontgomery(b3, Y3, _P, _Pprime));

	X3 = bigReplace(X3, bigMulModMontgomery(t4, Y3, _P, _Pprime));
	t2 = bigReplace(t2, bigMulModMontgomery(t3, t1, _P, _Pprime));
	X3 = bigReplace(X3, bigSubMod(t2, X3, _P));

	Y3 = bigReplace(Y3, bigMulModMontgomery(Y3, t0, _P, _Pprime));
	t1 = bigReplace(t1, bigMulModMontgomery(t1, Z3, _P, _Pprime));
	Y3 = bigReplace(Y3, bigAddMod(t1, Y3, _P));

	t0 = bigReplace(t0, bigMulModMontgomery(t0, t3, _P, _Pprime));
	Z3 = bigReplace(Z3, bigMulModMontgomery(Z3, t4, _P, _Pprime));
	Z3 = bigReplace(Z3, bigAddMod(Z3, t0, _P));
	bigRelease(t0);
	bigRelease(t1);
	bigRelease(t2);
	bigRelease(t3);
	bigRelease(t4);
	*X = X3; *Y = Y3; *Z = Z3;
}

void weierstrassA0Double(bignum _P, bignum _Pprime, bignum b3,
	bignum X, bignum Y, bignum Z,
	bignum* XR, bignum* YR, bignum* ZR
	)
{
	bignum t0, t1, t2, X3, Y3, Z3;
	if (BIGNUM_IS_NULL(Z)) {
		*XR = bigCopy(X);
		*YR = bigCopy(Y);
		*ZR = bigCopy(Z);
		return;
	}

	t0 = bigSquareModMontgomery(Y, _P, _Pprime);
	Z3 = bigAddMod(t0, t0, _P);
	Z3 = bigReplace(Z3, bigAddMod(Z3, Z3, _P));

	Z3 = bigReplace(Z3, bigAddMod(Z3, Z3, _P));
	t1 = bigMulModMontgomery(Y, Z, _P, _Pprime);
	t2 = bigSquareModMontgomery(Z, _P, _Pprime);

	t2 = bigReplace(t2, bigMulModMontgomery(b3, t2, _P, _Pprime));
	X3 = bigMulModMontgomery(t2, Z3, _P, _Pprime);
	Y3 = bigAddMod(t0, t2, _P);

	Z3 = bigReplace(Z3, bigMulModMontgomery(t1, Z3, _P, _Pprime));
	t1 = bigReplace(t1, bigAddMod(t2, t2, _P));
	t2 = bigReplace(t2, bigAddMod(t1, t2, _P));

	t0 = bigReplace(t0, bigSubMod(t0, t2, _P));
	Y3 = bigReplace(Y3, bigMulModMontgomery(t0, Y3, _P, _Pprime));
	Y3 = bigReplace(Y3, bigAddMod(X3, Y3, _P));

	t1 = bigReplace(t1, bigMulModMontgomery(X, Y, _P, _Pprime));
	X3 = bigReplace(X3, bigMulModMontgomery(t0, t1, _P, _Pprime));
	X3 = bigReplace(X3, bigAddMod(X3, X3, _P));
	bigRelease(t0);
	bigRelease(t1);
	bigRelease(t2);
	*XR = X3; *YR = Y3; *ZR = Z3;
}

void weierstrassA3Add(bignum _P, bignum _Pprime, bignum b,
	bignum X1, bignum Y1, bignum Z1,
	bignum X2, bignum Y2, bignum Z2,
	bignum* X, bignum* Y, bignum* Z)
{
	bignum t0, t1, t2, t3, t4, X3, Y3, Z3;
	if (BIGNUM_IS_NULL(Z1)) {
		*X = bigCopy(X2);
		*Y = bigCopy(Y2);
		*Z = bigCopy(Z2);
		return;
	}
	if (BIGNUM_IS_NULL(Z2)) {
		*X = bigCopy(X1);
		*Y = bigCopy(Y1);
		*Z = bigCopy(Z1);
		return;
	}

	t0 = bigMulModMontgomery(X1, X2, _P, _Pprime);
	t1 = bigMulModMontgomery(Y1, Y2, _P, _Pprime);
	t2 = bigMulModMontgomery(Z1, Z2, _P, _Pprime);

	t3 = bigAddMod(X1, Y1, _P);
	t4 = bigAddMod(X2, Y2, _P);
	t3 = bigReplace(t3, bigMulModMontgomery(t3, t4, _P, _Pprime));

	t4 = bigReplace(t4, bigAddMod(t0, t1, _P));
	t3 = bigReplace(t3, bigSubMod(t3, t4, _P));
	t4 = bigReplace(t4, bigAddMod(Y1, Z1, _P));

	X3 = bigAddMod(Y2, Z2, _P);
	t4 = bigReplace(t4, bigMulModMontgomery(t4, X3, _P, _Pprime));
	X3 = bigReplace(X3, bigAddMod(t1, t2, _P));

	t4 = bigReplace(t4, bigSubMod(t4, X3, _P));
	X3 = bigReplace(X3, bigAddMod(X1, Z1, _P));
	Y3 = bigAddMod(X2, Z2, _P);

	X3 = bigReplace(X3, bigMulModMontgomery(X3, Y3, _P, _Pprime));
	Y3 = bigReplace(Y3, bigAddMod(t0, t2, _P));
	Y3 = bigReplace(Y3, bigSubMod(X3, Y3, _P));

	Z3 = bigMulModMontgomery(b, t2, _P, _Pprime);
	X3 = bigReplace(X3, bigSubMod(Y3, Z3, _P));
	Z3 = bigReplace(Z3, bigAddMod(X3, X3, _P));

	X3 = bigReplace(X3, bigAddMod(X3, Z3, _P));
	Z3 = bigReplace(Z3, bigSubMod(t1, X3, _P));
	X3 = bigReplace(X3, bigAddMod(t1, X3, _P));

	Y3 = bigReplace(Y3, bigMulModMontgomery(b, Y3, _P, _Pprime));
	t1 = bigReplace(t1, bigAddMod(t2, t2, _P));
	t2 = bigReplace(t2, bigAddMod(t1, t2, _P));

	Y3 = bigReplace(Y3, bigSubMod(Y3, t2, _P));
	Y3 = bigReplace(Y3, bigSubMod(Y3, t0, _P));
	t1 = bigReplace(t1, bigAddMod(Y3, Y3, _P));

	Y3 = bigReplace(Y3, bigAddMod(t1, Y3, _P));
	t1 = bigReplace(t1, bigAddMod(t0, t0, _P));
	t0 = bigReplace(t0, bigAddMod(t1, t0, _P));

	t0 = bigReplace(t0, bigSubMod(t0, t2, _P));
	t1 = bigReplace(t1, bigMulModMontgomery(t4, Y3, _P, _Pprime));
	t2 = bigReplace(t2, bigMulModMontgomery(t0, Y3, _P, _Pprime));

	Y3 = bigReplace(Y3, bigMulModMontgomery(X3, Z3, _P, _Pprime));
	Y3 = bigReplace(Y3, bigAddMod(Y3, t2, _P));
	X3 = bigReplace(X3, bigMulModMontgomery(t3, X3, _P, _Pprime));

	X3 = bigReplace(X3, bigSubMod(X3, t1, _P));
	Z3 = bigReplace(Z3, bigMulModMontgomery(t4, Z3, _P, _Pprime));
	t1 = bigReplace(t1, bigMulModMontgomery(t3, t0, _P, _Pprime));

	Z3 = bigReplace(Z3, bigAddMod(Z3, t1, _P));
	bigRelease(t0);
	bigRelease(t1);
	bigRelease(t2);
	bigRelease(t3);
	bigRelease(t4);
	*X = X3; *Y = Y3; *Z = Z3;
}

void weierstrassA3Double(bignum _P, bignum _Pprime, bignum b,
	bignum X, bignum Y, bignum Z,
	bignum* XR, bignum* YR, bignum* ZR
)
{
	bignum t0, t1, t2, t3, X3, Y3, Z3;
	if (BIGNUM_IS_NULL(Z)) {
		*XR = bigCopy(X);
		*YR = bigCopy(Y);
		*ZR = bigCopy(Z);
		return;
	}

	t0 = bigSquareModMontgomery(X, _P, _Pprime);
	t1 = bigSquareModMontgomery(Y, _P, _Pprime);
	t2 = bigSquareModMontgomery(Z, _P, _Pprime);

	t3 = bigMulModMontgomery(X, Y, _P, _Pprime);
	t3 = bigReplace(t3, bigAddMod(t3, t3, _P));
	Z3 = bigMulModMontgomery(X, Z, _P, _Pprime);

	Z3 = bigReplace(Z3, bigAddMod(Z3, Z3, _P));
	Y3 = bigMulModMontgomery(b, t2, _P, _Pprime);
	Y3 = bigReplace(Y3, bigSubMod(Y3, Z3, _P));

	X3 = bigAddMod(Y3, Y3, _P);
	Y3 = bigReplace(Y3, bigAddMod(X3, Y3, _P));
	X3 = bigReplace(X3, bigSubMod(t1, Y3, _P));

	Y3 = bigReplace(Y3, bigAddMod(t1, Y3, _P));
	Y3 = bigReplace(Y3, bigMulModMontgomery(X3, Y3, _P, _Pprime));
	X3 = bigReplace(X3, bigMulModMontgomery(X3, t3, _P, _Pprime));

	t3 = bigReplace(t3, bigAddMod(t2, t2, _P));
	t2 = bigReplace(t2, bigAddMod(t2, t3, _P));
	Z3 = bigReplace(Z3, bigMulModMontgomery(b, Z3, _P, _Pprime));

	Z3 = bigReplace(Z3, bigSubMod(Z3, t2, _P));
	Z3 = bigReplace(Z3, bigSubMod(Z3, t0, _P));
	t3 = bigReplace(t3, bigAddMod(Z3, Z3, _P));

	Z3 = bigReplace(Z3, bigAddMod(Z3, t3, _P));
	t3 = bigReplace(t3, bigAddMod(t0, t0, _P));
	t0 = bigReplace(t0, bigAddMod(t3, t0, _P));

	t0 = bigReplace(t0, bigSubMod(t0, t2, _P));
	t0 = bigReplace(t0, bigMulModMontgomery(t0, Z3, _P, _Pprime));
	Y3 = bigReplace(Y3, bigAddMod(Y3, t0, _P));

	t0 = bigReplace(t0, bigMulModMontgomery(Y, Z, _P, _Pprime));
	t0 = bigReplace(t0, bigAddMod(t0, t0, _P));
	Z3 = bigReplace(Z3, bigMulModMontgomery(t0, Z3, _P, _Pprime));

	X3 = bigReplace(X3, bigSubMod(X3, Z3, _P));
	Z3 = bigReplace(Z3, bigMulModMontgomery(t0, t1, _P, _Pprime));
	Z3 = bigReplace(Z3, bigAddMod(Z3, Z3, _P));

	Z3 = bigReplace(Z3, bigAddMod(Z3, Z3, _P));

	bigRelease(t0);
	bigRelease(t1);
	bigRelease(t2);
	bigRelease(t3);
	*XR = X3; *YR = Y3; *ZR = Z3;
}

void weierstrassA0Mul(bignum _P, bignum _Pprime, bignum b3, int constantTime, bignum s, bignum px, bignum py, bignum pz, bignum* x, bignum* y, bignum* z)
{
	LINT nbits = bigNbits(_P);
	LINT i;
	bignum qx = bigFromInt(0);
	bignum qy = bigFromInt(0);
	bignum qz = bigFromInt(0);
	px = bigMod(px, _P);
	py = bigMod(py, _P);
	pz = bigMod(pz, _P);
	if (constantTime) {
		for (i = 0; i < nbits; i++) {
			bignum px2, py2, pz2;
			bignum qx2, qy2, qz2;
			LINT bit = bigBit(s, i);
			weierstrassA0Add(_P, _Pprime, b3, qx, qy, qz, px, py, pz, &qx2, &qy2, &qz2);
			qx = bigSelectOrRelease(bit, qx, qx2);
			qy = bigSelectOrRelease(bit, qy, qy2);
			qz = bigSelectOrRelease(bit, qz, qz2);
			weierstrassA0Double(_P, _Pprime, b3, px, py, pz, &px2, &py2, &pz2);
			px = bigReplace(px, px2);
			py = bigReplace(py, py2);
			pz = bigReplace(pz, pz2);
		}
	}
	else {
		for (i = 0; i < nbits; i++) {
			bignum px2, py2, pz2;
			if (bigBit(s, i)) {
				bignum qx2, qy2, qz2;
				weierstrassA0Add(_P, _Pprime, b3, qx, qy, qz, px, py, pz, &qx2, &qy2, &qz2);
				qx = bigReplace(qx, qx2);
				qy = bigReplace(qy, qy2);
				qz = bigReplace(qz, qz2);
			}
			weierstrassA0Double(_P, _Pprime, b3, px, py, pz, &px2, &py2, &pz2);
			px = bigReplace(px, px2);
			py = bigReplace(py, py2);
			pz = bigReplace(pz, pz2);
		}

	}
	bigRelease(px);
	bigRelease(py);
	bigRelease(pz);
	*x = qx; *y = qy; *z = qz;
	//fun _ecMulJacobian(constantTime, curve, n, p)=
	//let bigNbits(n)->nbits in
	//	let nil->result in(
	//		for i = 0; i < nbits do (
	//			// constant time:
	//			if constantTime then set result = selectConstantTime(bigBit(n, i), result, call curve._add(curve, result, p))
	//				// faster yet not constant time:
	//			else if bigBit(n, i) == 1 then set result = call curve._add(curve, result, p);
	//set p = call curve._double(curve, p);
	//	);
	//result
}
void weierstrassA3Mul(bignum _P, bignum _Pprime, bignum b, int constantTime, bignum s, bignum px, bignum py, bignum pz, bignum* x, bignum* y, bignum* z)
{
	LINT nbits = bigNbits(_P);
	LINT i;
	bignum qx = bigFromInt(0);
	bignum qy = bigFromInt(0);
	bignum qz = bigFromInt(0);
	px = bigMod(px, _P);
	py = bigMod(py, _P);
	pz = bigMod(pz, _P);
	if (constantTime) {
		for (i = 0; i < nbits; i++) {
			bignum px2, py2, pz2;
			bignum qx2, qy2, qz2;
			LINT bit = bigBit(s, i);
			weierstrassA3Add(_P, _Pprime, b, qx, qy, qz, px, py, pz, &qx2, &qy2, &qz2);
			qx = bigSelectOrRelease(bit, qx, qx2);
			qy = bigSelectOrRelease(bit, qy, qy2);
			qz = bigSelectOrRelease(bit, qz, qz2);
			weierstrassA3Double(_P, _Pprime, b, px, py, pz, &px2, &py2, &pz2);
			px = bigReplace(px, px2);
			py = bigReplace(py, py2);
			pz = bigReplace(pz, pz2);
		}
	}
	else {
		for (i = 0; i < nbits; i++) {
			bignum px2, py2, pz2;
			if (bigBit(s, i)) {
				bignum qx2, qy2, qz2;
				weierstrassA3Add(_P, _Pprime, b, qx, qy, qz, px, py, pz, &qx2, &qy2, &qz2);
				qx = bigReplace(qx, qx2);
				qy = bigReplace(qy, qy2);
				qz = bigReplace(qz, qz2);
			}
			weierstrassA3Double(_P, _Pprime, b, px, py, pz, &px2, &py2, &pz2);
			px = bigReplace(px, px2);
			py = bigReplace(py, py2);
			pz = bigReplace(pz, pz2);
		}

	}
	bigRelease(px);
	bigRelease(py);
	bigRelease(pz);
	*x = qx; *y = qy; *z = qz;
}

int bigWeierstrassA0Add(bignum _P, bignum _Pprime, bignum b3,
	bignum X1, bignum Y1, bignum Z1,
	bignum X2, bignum Y2, bignum Z2,
	bignum* X, bignum* Y, bignum* Z)
{
	if (bigSign(_P) || bigSign(_Pprime) || (bigSign(b3))) return -1;
	if ((bigNbits(_P) > 521) || (bigNbits(_Pprime) > 521) || (bigNbits(b3) > 521)) return -1;
	if (bigSign(X1) || bigSign(Y1) || bigSign(Z1) || bigSign(X2) || bigSign(Y2) || bigSign(Z2)) return -1;
	if (BIGNUM_GREATER_EQUAL(X1, _P) || BIGNUM_GREATER_EQUAL(Y1, _P) || BIGNUM_GREATER_EQUAL(Z1, _P)) return -1;
	if (BIGNUM_GREATER_EQUAL(X2, _P) || BIGNUM_GREATER_EQUAL(Y2, _P) || BIGNUM_GREATER_EQUAL(Z2, _P)) return -1;

	weierstrassA0Add(_P, _Pprime, b3, X1, Y1, Z1, X2, Y2, Z2, X, Y, Z);
	return 0;
}
int bigWeierstrassA0Mul(bignum _P, bignum _Pprime, bignum b3, int constantTime, bignum s, bignum px, bignum py, bignum pz, bignum* x, bignum* y, bignum* z)
{
	if (bigSign(_P)||bigSign(_Pprime) || (bigSign(b3))) return -1;
	if ((bigNbits(_P) > 521) || (bigNbits(_Pprime) > 521) || (bigNbits(b3) > 521)) return -1;
	weierstrassA0Mul(_P, _Pprime, b3, constantTime, s, px, py, pz, x, y, z);
	return 0;
}

int bigWeierstrassA3Add(bignum _P, bignum _Pprime, bignum b,
	bignum X1, bignum Y1, bignum Z1,
	bignum X2, bignum Y2, bignum Z2,
	bignum* X, bignum* Y, bignum* Z)
{
	if (bigSign(_P) || bigSign(_Pprime) || (bigSign(b))) return -1;
	if ((bigNbits(_P) > 521) || (bigNbits(_Pprime) > 521) || (bigNbits(b) > 521)) return -1;
	if (bigSign(X1) || bigSign(Y1) || bigSign(Z1) || bigSign(X2) || bigSign(Y2) || bigSign(Z2)) return -1;
	if (BIGNUM_GREATER_EQUAL(X1, _P) || BIGNUM_GREATER_EQUAL(Y1, _P) || BIGNUM_GREATER_EQUAL(Z1, _P)) return -1;
	if (BIGNUM_GREATER_EQUAL(X2, _P) || BIGNUM_GREATER_EQUAL(Y2, _P) || BIGNUM_GREATER_EQUAL(Z2, _P)) return -1;
	weierstrassA3Add(_P, _Pprime, b, X1, Y1, Z1, X2, Y2, Z2, X, Y, Z);
	return 0;
}
int bigWeierstrassA3Mul(bignum _P, bignum _Pprime, bignum b, int constantTime, bignum s, bignum px, bignum py, bignum pz, bignum* x, bignum* y, bignum* z)
{
	if (bigSign(_P) || bigSign(_Pprime) || (bigSign(b))) return -1;
	if ((bigNbits(_P) > 521) || (bigNbits(_Pprime) > 521) || (bigNbits(b) > 521)) return -1;
	weierstrassA3Mul(_P, _Pprime, b, constantTime, s, px, py, pz, x, y, z);
	return 0;
}

