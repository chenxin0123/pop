/*！
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UnitBezier_h
#define UnitBezier_h

#include <math.h>

namespace WebCore {
  ///保存贝塞尔曲线的多项式系数
  struct UnitBezier {
    /*
     p1,p2 分别是两个点（p1x,p1y）(p2x,p2y),相当于传入两个点，在计算的过程中默认开始点（0，0）和 结束点（1，1），一共四个点，来确定B(t)曲线方程。假设开始结束的两个点分别为p0、p3，
     B(t) = p0 * (1 - t) ^ 3 + 3 * p1 * t (1 - t)^2 + 3 * p2 * t^2 * (1 - t) + p3 * t^3 ,   t属于[0,1];
     由于点 p0（0,0），公式可以转化为
     B(t) = 3 * p1 * t * (1-t)^2 + 3 * p2 * t^2 * (1-t)  +  p3 * t^3 ,    t 属于[0,1] ,
     再合并同类项得到如下曲线方程
     B(t) = (3 * p1 - 3 * p2 + p3) * t^3  + (3 * p2 - 6 * p1) * t^2 + 3 * p1 * t;
     这里可以用ax,bx,cx; ay,by,cy 分别表示 x轴 ，y 轴的多项式系数，即
     Bx(t) = ax * t^3  + bx * t^2 + cx * t;
     By(t) = ay * t^3  + by * t^2 + cy * t;
     由于点 p3 为 （1，1） 因此
     cx =  3 * p1x;
     
     bx =  3 * p2x - 6 * p1x
     =  3 * p2x  - 3 * p1x  - 3 * p1x
     =  3 * p2x - 3 * p1x - cx
     =  3 * (p2 x- p1x) - cx
     
     ax =  3 * p1x - 3 * p2x + p3x
     =  3 * p1x - 3 * p2x + 1
     =  1 - cx - bx
     
     同理
     cy =  3 * p1y
     by =  3 * (p2y - p1y) - cy
     ay =  1 - cy - by
     */
    UnitBezier(double p1x, double p1y, double p2x, double p2y)
    {
      // Calculate the polynomial coefficients, implicit first and last control points are (0,0) and (1,1).
      cx = 3.0 * p1x;
      bx = 3.0 * (p2x - p1x) - cx;
      ax = 1.0 - cx -bx;
      
      cy = 3.0 * p1y;
      by = 3.0 * (p2y - p1y) - cy;
      ay = 1.0 - cy - by;
    }
    ///X坐标 霍纳法则
    double sampleCurveX(double t)
    {
      // `ax t^3 + bx t^2 + cx t' expanded using Horner's rule.
      return ((ax * t + bx) * t + cx) * t;
    }
    ///Y坐标
    double sampleCurveY(double t)
    {
      return ((ay * t + by) * t + cy) * t;
    }
    
    //导数 一个函数在某一点的导数描述了这个函数在这一点附近的变化率
    double sampleCurveDerivativeX(double t)
    {
     //3*ax*t^2 + 2*bx*t + cx
      return (3.0 * ax * t + 2.0 * bx) * t + cx;
    }
    
    // Given an x value, find a parametric value it came from.
    // 给定一个x值 返回对应的t值 epsilon误差值
    double solveCurveX(double x, double epsilon)
    {
      double t0;
      double t1;
      double t2;
      double x2;
      double d2;
      int i;
      
      //先采用牛顿迭代法求t ，一共迭代8次，成功则返回结果，如果失败则继续下边二分法求解
      // First try a few iterations of Newton's method -- normally very fast.
      for (t2 = x, i = 0; i < 8; i++) {
        x2 = sampleCurveX(t2) - x;
        if (fabs (x2) < epsilon)
          return t2;
        d2 = sampleCurveDerivativeX(t2);
        if (fabs(d2) < 1e-6)
          break;
        t2 = t2 - x2 / d2;
      }
      
      // Fall back to the bisection method for reliability.
      t0 = 0.0;
      t1 = 1.0;
      t2 = x;
      
      if (t2 < t0)
        return t0;
      if (t2 > t1)
        return t1;
      
      while (t0 < t1) {
        x2 = sampleCurveX(t2);
        if (fabs(x2 - x) < epsilon)
          return t2;
        if (x > x2)
          t0 = t2;
        else
          t1 = t2;
        t2 = (t1 - t0) * .5 + t0;
      }
      
      // Failure.
      return t2;
    }
    
    double solve(double x, double epsilon)
    {
      return sampleCurveY(solveCurveX(x, epsilon));
    }
    
  private://三个多项式系数
    double ax;
    double bx;
    double cx;
    
    double ay;
    double by;
    double cy;
  };
}
#endif
