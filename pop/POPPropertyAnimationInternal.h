/**!
 Copyright (c) 2014-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#import "POPAnimationInternal.h"
#import "POPPropertyAnimation.h"

static void clampValue(CGFloat &value, CGFloat fromValue, CGFloat toValue, NSUInteger clamp)
{
  BOOL increasing = (toValue > fromValue);

  // Clamp start of animation.
  if ((kPOPAnimationClampStart & clamp) &&
      ((increasing && (value < fromValue)) || (!increasing && (value > fromValue)))) {
    value = fromValue;
  }

  // Clamp end of animation.
  if ((kPOPAnimationClampEnd & clamp) &&
      ((increasing && (value > toValue)) || (!increasing && (value < toValue)))) {
    value = toValue;
  }
}

struct _POPPropertyAnimationState : _POPAnimationState
{
  POPAnimatableProperty *property;
  POPValueType valueType;//值类型
  NSUInteger valueCount;//值个数
  VectorRef fromVec;
  VectorRef toVec;
  VectorRef currentVec;//当前值
  VectorRef previousVec;//上个值
  VectorRef previous2Vec;//上上个值
  VectorRef velocityVec;//速度
  VectorRef originalVelocityVec;
  VectorRef distanceVec;
  CGFloat roundingFactor;
  NSUInteger clampMode;
  NSArray *progressMarkers;//NSNumber数组
  POPProgressMarker *progressMarkerState;//POPProgressMarker数组 根据progressMarkers生成
  NSUInteger progressMarkerCount;
  NSUInteger nextProgressMarkerIdx;//下一个需要通知的进度的索引
  CGFloat dynamicsThreshold;

  _POPPropertyAnimationState(id __unsafe_unretained anim) : _POPAnimationState(anim),
  property(nil),
  valueType((POPValueType)0),//kPOPValueUnknown
  valueCount(0),
  fromVec(nullptr),
  toVec(nullptr),
  currentVec(nullptr),
  previousVec(nullptr),
  previous2Vec(nullptr),
  velocityVec(nullptr),
  originalVelocityVec(nullptr),
  distanceVec(nullptr),
  roundingFactor(0),
  clampMode(0),
  progressMarkers(nil),
  progressMarkerState(nil),
  progressMarkerCount(0),
  nextProgressMarkerIdx(0),
  dynamicsThreshold(0)
  {
    type = kPOPAnimationBasic;
  }

  ~_POPPropertyAnimationState()
  {
    if (progressMarkerState) {
      free(progressMarkerState);
      progressMarkerState = NULL;
    }
  }

  bool canProgress() {
    return hasValue();
  }

  bool shouldRound() {
    return 0 != roundingFactor;
  }

    //值的个数
  bool hasValue() {
    return 0 != valueCount;
  }

  bool isDone() {
    // inherit done
    if (_POPAnimationState::isDone()) {
      return true;
    }

    // consider an animation with no values done
    if (!hasValue() && !isCustom()) {
      return true;
    }

    return false;
  }

  // returns a copy of the currentVec, rounding if needed 动画当前值
  VectorRef currentValue() {
    VectorRef vec = VectorRef(Vector::new_vector(currentVec.get()));
      //像素对齐
    if (shouldRound()) {
      vec->subRound(1 / roundingFactor);
    }
      return vec;
  }

    // 重置ProgressMarkerState 的状态  不清空
  void resetProgressMarkerState()
  {
    for (NSUInteger idx = 0; idx < progressMarkerCount; idx++)
      progressMarkerState[idx].reached = false;

    nextProgressMarkerIdx = 0;
  }

    //释放progressMarkerState内存 根据progressMarkers重新申请内存创建新的progressMarkerState
  void updatedProgressMarkers()
  {
    if (progressMarkerState) {
      free(progressMarkerState);
      progressMarkerState = NULL;
    }

    progressMarkerCount = progressMarkers.count;

    if (0 != progressMarkerCount) {
      progressMarkerState = (POPProgressMarker *)malloc(progressMarkerCount * sizeof(POPProgressMarker));
      [progressMarkers enumerateObjectsUsingBlock:^(NSNumber *progressMarker, NSUInteger idx, BOOL *stop) {
        progressMarkerState[idx].reached = false;
        progressMarkerState[idx].progress = [progressMarker floatValue];
      }];
    }

    nextProgressMarkerIdx = 0;
  }

  virtual void updatedDynamicsThreshold()
  {
    dynamicsThreshold = property.threshold;
  }

  // currentVec = toVec | delegate to value | clamp
  void finalizeProgress()
  {
    progress = 1.0;
    NSUInteger count = valueCount;
    VectorRef outVec(Vector::new_vector(count, NULL));

    if (outVec && toVec) {
      *outVec = *toVec;
    }

    currentVec = outVec;
    clampCurrentValue();
    delegateProgress();
  }

  // 计算动画进度
  void computeProgress() {
    if (!canProgress()) {
      return;
    }

    static ComputeProgressFunctor<Vector4r> func;
    Vector4r v = vector4(currentVec);
    Vector4r f = vector4(fromVec);
    Vector4r t = vector4(toVec);
    progress = func(v, f, t);
  }

  // 处理progressMarkerState handleDidReachToValue
  void delegateProgress() {
    if (!canProgress()) {
      return;
    }

      // 处理剩余的progressMarkerState
    if (delegateDidProgress && progressMarkerState) {

        // 遍历nextProgressMarkerIdx之后的值
      while (nextProgressMarkerIdx < progressMarkerCount) {
        if (progress < progressMarkerState[nextProgressMarkerIdx].progress)
          break;

        if (!progressMarkerState[nextProgressMarkerIdx].reached) {
          ActionEnabler enabler;//RAII
          [delegate pop_animation:self didReachProgress:progressMarkerState[nextProgressMarkerIdx].progress];
          progressMarkerState[nextProgressMarkerIdx].reached = true;
        }

        nextProgressMarkerIdx++;
      }
    }

      // valueCount == 0 YES || toVec-currentVec == 0 YES
    if (!didReachToValue) {
      bool didReachToValue = false;
      if (0 == valueCount) {
        didReachToValue = true;
      } else {
          // 到完成的距离
        Vector4r distance = toVec->vector4r();
        distance -= currentVec->vector4r();

          // 完成
        if (0 == distance.squaredNorm()) {
          didReachToValue = true;
        } else {
          // components
          if (distanceVec) {
            didReachToValue = true;
            const CGFloat *distanceValues = distanceVec->data();
            for (NSUInteger idx = 0; idx < valueCount; idx++) {
              didReachToValue &= (signbit(distance[idx]) != signbit(distanceValues[idx]));
            }
          }
        }
      }

      if (didReachToValue) {
        handleDidReachToValue();
      }
    }
  }

  // delete and block and tracer record
  void handleDidReachToValue() {
    didReachToValue = true;

    if (delegateDidReachToValue) {
      ActionEnabler enabler;
      [delegate pop_animationDidReachToValue:self];
    }

    POPAnimationDidReachToValueBlock block = animationDidReachToValueBlock;
    if (block != NULL) {
      ActionEnabler enabler;
      block(self);
    }

    if (tracing) {
      [tracer didReachToValue:POPBox(currentValue(), valueType, true)];
    }
  }

    // read and record
  void readObjectValue(VectorRef *ptrVec, id obj)
  {
    // use current object value as from value
    pop_animatable_read_block read = property.readBlock;
    if (NULL != read) {

      Vector4r vec = read_values(read, obj, valueCount);
      *ptrVec = VectorRef(Vector::new_vector(valueCount, vec));

      if (tracing) {
        [tracer readPropertyValue:POPBox(*ptrVec, valueType, true)];
      }
    }
  }

  // 每一帧都会调用 初始化 fromValue toValue currentValue velocity originalVelocity distanceVec
  // 保证各项均有值
  virtual void willRun(bool started, id obj) {
    // ensure from value initialized
    // 没有fromValue 将当前值作为fromValue
    if (NULL == fromVec) {
      readObjectValue(&fromVec, obj);
    }

    // ensure to value initialized
    // 没有toValue 如果是kPOPAnimationDecay调用动画的toValue自动计算最终值 否则以当前值为toValue
    if (NULL == toVec) {
      // compute decay to value
      if (kPOPAnimationDecay == type) {
        [self toValue];
      } else {
        // read to value
        readObjectValue(&toVec, obj);
      }
    }

    // handle one time value initialization on start
    if (started) {//动画刚开始才会执行这块代码 其他时候都是false

      // initialize current vec
      if (!currentVec) {
        currentVec = VectorRef(Vector::new_vector(valueCount, NULL));

        // initialize current value with from value
        // only do this on initial creation to avoid overwriting current value
        // on paused animation continuation
        if (currentVec && fromVec) {
          *currentVec = *fromVec;
        }
      }

      // ensure velocity values
      if (!velocityVec) {
        velocityVec = VectorRef(Vector::new_vector(valueCount, NULL));
      }
      if (!originalVelocityVec) {
        originalVelocityVec = VectorRef(Vector::new_vector(valueCount, NULL));
      }
    }

    // ensure distance value initialized
    // depends on current value set on one time start
    if (NULL == distanceVec) {

      // not yet started animations may not have current value
      VectorRef fromVec2 = NULL != currentVec ? currentVec : fromVec;

      if (fromVec2 && toVec) {
        Vector4r distance = toVec->vector4r();
        distance -= fromVec2->vector4r();

        if (0 != distance.squaredNorm()) {
          distanceVec = VectorRef(Vector::new_vector(valueCount, distance));
        }
      }
    }
  }

  // 重置
  virtual void reset(bool all) {
    _POPAnimationState::reset(all);

    if (all) {
      currentVec = NULL;
      previousVec = NULL;
      previous2Vec = NULL;
    }
    progress = 0;
    resetProgressMarkerState();
    didReachToValue = false;
    distanceVec = NULL;
  }

  // 防止值小于起始值 或者大于终点值 kPOPAnimationClampNone直接return
  void clampCurrentValue(NSUInteger clamp)
  {
    if (kPOPAnimationClampNone == clamp)
      return;

    // Clamp all vector values
    CGFloat *currentValues = currentVec->data();
    const CGFloat *fromValues = fromVec->data();
    const CGFloat *toValues = toVec->data();

    for (NSUInteger idx = 0; idx < valueCount; idx++) {
      clampValue(currentValues[idx], fromValues[idx], toValues[idx], clamp);
    }
  }

  void clampCurrentValue()
  {
    clampCurrentValue(clampMode);
  }
};

typedef struct _POPPropertyAnimationState POPPropertyAnimationState;

@interface POPPropertyAnimation ()

@end

