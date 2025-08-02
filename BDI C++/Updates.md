diff --git a//dev/null b/BDI C++/Updates.md	
index 0000000000000000000000000000000000000000..23573fd7ed31e52e98df58ea1d41595918842ff6 100644
--- a//dev/null
+++ b/BDI C++/Updates.md	
@@ -0,0 +1,27 @@
+# Development Notes
+
+This document tracks pending source file updates and considerations in the BDI C++ codebase.
+
+## TypeSystem.cpp
+
+Currently, all methods in `TypeSystem.hpp` are static inline or basic checks, so a separate `.cpp` file is not needed unless more complex, non-template logic or state (such as a dynamic type registry) is introduced.
+
+## TypedPayload.cpp
+
+`TypedPayload.hpp` similarly relies on templates and simple structs. The tricky part is the C++ type to `BDIType` mapping within `createFrom<T>`, which is best kept in the header. A separate `.cpp` would only be necessary if non-template helpers are added.
+
+## BDINode.cpp
+
+`BDINode.cpp` should include:
+
+```cpp
+#include "BDINode.hpp"
+#include "BDIGraph.hpp"          // validation context
+#include "../types/TypeSystem.hpp"  // type compatibility checks
+
+namespace bdi::core::graph {
+    // Implementation goes here.
+}
+```
+
+This file would provide implementations for `BDINode` functions that require access to the graph context or the type system for validation.
