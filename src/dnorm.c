#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdbool.h>
#include <limits.h>
#include "bmath.h"

double dnorm4(double x, double mu, double sigma, int give_log) {
#ifdef IEEE_754
    if (ISNAN(x) || ISNAN(mu) || ISNAN(sigma))
        return x + mu + sigma;
#endif
    if (!R_FINITE(sigma)) return R_D__0;
    if (!R_FINITE(x) && mu == x) return ML_NAN; /* x-mu is NaN */
    if (sigma <= 0.0) {
        if (sigma < 0) ML_ERR_return_NAN;
        /* sigma == 0 */
        return (x == mu) ? ML_POSINF : R_D__0;
    } else {
        x = (x - mu) / sigma;
    }
    if (!R_FINITE(x)) return R_D__0;

    x = fabs(x);
    if (x >= 2 * sqrt(DBL_MAX)) return R_D__0;
    if (give_log)
        return -(M_LN_SQRT_2PI + 0.5 * x * x + log(sigma));
    //  M_1_SQRT_2PI = 1 / sqrt(2 * pi)
    // more accurate, less fast :
    if (x < 5 && fabs(sigma) > 1.0e-15) return M_1_SQRT_2PI * exp(-0.5 * x * x) / sigma;

    /* ELSE:

     * x*x  may lose upto about two digits accuracy for "large" x
     * Morten Welinder's proposal for PR#15620
     * https://bugs.r-project.org/bugzilla/show_bug.cgi?id=15620

     * -- 1 --  No hoop jumping when we underflow to zero anyway:

     *  -x^2/2 <         log(2)*.Machine$double.min.exp  <==>
     *     x   > sqrt(-2*log(2)*.Machine$double.min.exp) =IEEE= 37.64031
     * but "thanks" to denormalized numbers, underflow happens a bit later,
     *  effective.D.MIN.EXP <- with(.Machine, double.min.exp + double.ulp.digits)
     * for IEEE, DBL_MIN_EXP is -1022 but "effective" is -1074
     * ==> boundary = sqrt(-2*log(2)*(.Machine$double.min.exp + .Machine$double.ulp.digits))
     *              =IEEE=  38.58601
     * [on one x86_64 platform, effective boundary a bit lower: 38.56804]
     */
    if (x > sqrt(-2 * M_LN2 * (DBL_MIN_EXP + 1 - DBL_MANT_DIG))) return 0.;

    /* Now, to get full accurary, split x into two parts,
     *  x = x1+x2, such that |x2| <= 2^-16.
     * Assuming that we are using IEEE doubles, that means that
     * x1*x1 is error free for x<1024 (but we have x < 38.6 anyway).

     * If we do not have IEEE this is still an improvement over the naive formula.
     */
    double x1 = //  R_forceint(x * 65536) / 65536 =
            ldexp(R_forceint(ldexp(x, 16)), -16);
    double x2 = x - x1;
    double result;
    if (fabs(sigma) > 1e-25)
        result =  M_1_SQRT_2PI / sigma *
            (exp(-0.5 * x1 * x1) * exp((-0.5 * x2 - x1) * x2));
    else
        result = NAN;
    return result;
}
