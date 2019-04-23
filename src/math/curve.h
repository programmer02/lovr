#include "util.h"
#include "types.h"
#include "lib/vec/vec.h"

typedef struct {
  Ref ref;
  vec_float_t points;
} Curve;

Curve* lovrCurveInit(Curve* curve, usize sizeHint);
#define lovrCurveCreate(...) lovrCurveInit(lovrAlloc(Curve), __VA_ARGS__)
void lovrCurveDestroy(void* ref);
void lovrCurveEvaluate(Curve* curve, f32 t, f32 point[3]);
void lovrCurveGetTangent(Curve* curve, f32 t, f32 point[3]);
void lovrCurveRender(Curve* curve, f32 t1, f32 t2, f32 points[3], usize n);
Curve* lovrCurveSlice(Curve* curve, f32 t1, f32 t2);
usize lovrCurveGetPointCount(Curve* curve);
void lovrCurveGetPoint(Curve* curve, usize index, f32 point[3]);
void lovrCurveSetPoint(Curve* curve, usize index, f32 point[3]);
void lovrCurveAddPoint(Curve* curve, f32 point[3], usize index);
void lovrCurveRemovePoint(Curve* curve, usize index);
