/**!
 Copyright (c) 2014-present, Facebook, Inc.
 All rights reserved.
 
 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#import "POPMath.h"

#import "POPAnimationPrivate.h"
#import "UnitBezier.h"

///向量插值线性插值 v(t) = v1 + t*(v2-v1)（0<=t<=1)（因为t是一次方的，所以是线性的。）
void POPInterpolateVector(NSUInteger count, CGFloat *dst, const CGFloat *from, const CGFloat *to, CGFloat f)
{
  for (NSUInteger idx = 0; idx < count; idx++) {
    dst[idx] = MIX(from[idx], to[idx], f);
  }
}

///获得贝塞尔曲线的Y坐标
double POPTimingFunctionSolve(const double vec[4], double t, double eps)
{
  WebCore::UnitBezier bezier(vec[0], vec[1], vec[2], vec[3]);
  return bezier.solve(t, eps);
}

///value在startValue和endValue之间所占的比例 0-1
double POPNormalize(double value, double startValue, double endValue)
{
  return (value - startValue) / (endValue - startValue);
}

//POPNormalize 反向计算
double POPProjectNormal(double n, double start, double end)
{
  return start + (n * (end - start));
}

///线性插值 v(t) = v1 + t*(v2-v1)
static double linear_interpolation(double t, double start, double end)
{
  return t * end + (1.f - t) * start;
}

//EaseOut 先快后慢
double POPQuadraticOutInterpolation(double t, double start, double end)
{
  return linear_interpolation(2*t - t*t, start, end);
}

static double b3_friction1(double x)
{
  return (0.0007 * pow(x, 3)) - (0.031 * pow(x, 2)) + 0.64 * x + 1.28;
}

static double b3_friction2(double x)
{
  return (0.000044 * pow(x, 3)) - (0.006 * pow(x, 2)) + 0.36 * x + 2.;
}

static double b3_friction3(double x)
{
  return (0.00000045 * pow(x, 3)) - (0.000332 * pow(x, 2)) + 0.1078 * x + 5.84;
}

//返回NoBounce需要的摩擦
double POPBouncy3NoBounce(double tension)
{
  double friction = 0;
  if (tension <= 18.) {
    friction = b3_friction1(tension);
  } else if (tension > 18 && tension <= 44) {
    friction = b3_friction2(tension);
  } else if (tension > 44) {
    friction = b3_friction3(tension);
  } else {
    assert(false);
  }
  return friction;
}

//解方程 a * x^2 + b * x + c = 0 公式解法
void POPQuadraticSolve(CGFloat a, CGFloat b, CGFloat c, CGFloat &x1, CGFloat &x2)
{
  CGFloat discriminant = sqrt(b * b - 4 * a * c);
  x1 = (-b + discriminant) / (2 * a);
  x2 = (-b - discriminant) / (2 * a);
}
