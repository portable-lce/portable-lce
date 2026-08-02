#include <cassert>

#include "IRenderPath.h"

namespace rp::render_path_internal {

static IRenderPath* s_active = nullptr;

void set_active(IRenderPath* path) { s_active = path; }

IRenderPath& get_active() {
    assert(s_active && "RenderPath accessed before set_active()");
    return *s_active;
}

}  // namespace rp::render_path_internal
