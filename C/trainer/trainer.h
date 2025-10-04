
#ifndef BDI_TRAINER_H
#define BDI_TRAINER_H

// Main trainer header - includes all Phase 6 components

// Automatic Differentiation
#include "autodiff/forward_ad.h"
#include "autodiff/reverse_ad.h"
#include "autodiff/gradient.h"

// Optimizers
#include "optimizers/sgd.h"
#include "optimizers/adam.h"
#include "optimizers/rmsprop.h"
#include "optimizers/lr_scheduler.h"

// Loss Functions
#include "loss/loss.h"

// Metrics
#include "metrics/metrics.h"

// Training Infrastructure
#include "training/training.h"

#endif // BDI_TRAINER_H
