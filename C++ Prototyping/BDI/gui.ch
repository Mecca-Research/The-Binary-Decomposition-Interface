    import bdios::memory; 
    import bdios::events; // For input events 
    import chimera::collections::HashMap; // For window map 
    import chimera::collections::LinkedList; // For z-order 
    namespace bdios::services::gui; 
    // --- Data Structures --- 
    struct Rect { x: i32; y: i32; width: i32; height: i32; } 
    struct Color { r: u8; g: u8; b: u8; a: u8; } 
    // Define DrawCommand variant/enum and structs if not already done 
    enum DrawCommandType { FILL_RECT, DRAW_BITMAP, DRAW_TEXT /* ... */ } 
    struct FillRectCmd { rect: Rect; color: Color; } 
    struct DrawBitmapCmd { dest_rect: Rect; source_region: MemoryRegion<Color>; source_rect: Rect; } 
    // struct DrawTextCmd { ... font info, text string, position, color ... } 
    using DrawCommand = variant<FillRectCmd, DrawBitmapCmd /*, DrawTextCmd*/>; // Chimera variant syntax needed 
    struct Window { 
        id: u64; 
        title: string; 
        bounds: Rect; // Position and size on screen (for compositor)
        framebuffer_region: MemoryRegion<Color>; // Region holding pixel data 
        is_dirty: bool; // Does compositor need to redraw this? 
        owner_task: TaskID; 
        draw_commands: LinkedList<DrawCommand>; // List of drawing ops for this window 
        // Widget tree or draw command list? 
    }
    enum DrawCommandType { FILL_RECT, DRAW_TEXT, DRAW_BITMAP } 
    struct DrawCommand { 
        type: DrawCommandType; 
        bounds: Rect; 
        color: Color; 
        text: string; // For DRAW_TEXT 
        bitmap_region_hash: HashValue; // For DRAW_BITMAP 
    }
    // --- Service State --- 
    var windows: HashMap<u64, Window>; 
    var z_order: LinkedList<u64>; // Window IDs in back-to-front order 
    var next_window_id: u64 = 1; 
    var gui_lock: Mutex; // Protect shared state 
    // Assume screen dimensions are known or obtained during init 
    var screen_width: i32 = 1024; 
    var screen_height: i32 = 768; 
    // Screen framebuffer region obtained from graphics driver service during init 
    var screen_framebuffer: MemoryRegion<Color>; 
    // var screen_framebuffer_region: MemoryRegion<Color>; // Acquired from graphics driver/HAL 
    // var compositor_task_id: TaskID; // Task responsible for drawing to screen 
    // --- Service Operations --- 
    @BDIOsService(id = GUI_SERVICE_ID, op = GUIOp::CREATE_WINDOW) 
    def create_window(title: string, width: i32, height: i32): u64 { 
        // Generate new window ID 
        // Allocate framebuffer region using Memory service 
        // Create Window struct 
        // Add to windows map and z_order list 
        // Return window ID 
        return 0; // Placeholder 
    }
    @BDIOsService(id = GUI_SERVICE_ID, op = GUIOp::DRAW_RECT) 
    def draw_rect(window_id: u64, rect: Rect, color: Color): bool { 
        // Find window 
        // Add FILL_RECT command to window's draw command list (or draw directly to buffer?) 
        // Mark window as dirty 
        // Signal compositor task? 
        return true;
    }
    // Add DRAW_TEXT, DRAW_BITMAP... 
    @BDIOsService(id = GUI_SERVICE_ID, op = GUIOp::GET_EVENTS) 
    def get_events(window_id: u64): Queue<UIEvent> { 
        // Get events targeted for this window from internal queue (populated by input handlers) 
        return Queue<UIEvent>::new(); // Placeholder 
    }
    // --- Compositor Logic (Runs as separate task) --- 
    def compositor_task(): void { 
        while(true) { 
            var needs_redraw = false; 
            // Check all windows, if any are dirty, set needs_redraw = true; break; 
            if (needs_redraw) { 
                // Clear screen framebuffer region (or parts needed) 
                // Iterate windows in z_order: 
//      Execute its draw commands (render text, blit bitmaps) into its buffer 
//      Copy (blit) window's dirty region to screen framebuffer region 
// Mark windows as clean 
// (Optional) Signal graphics driver to swap buffers (via OS_SERVICE_CALL) 
            } 
// Wait for redraw signal or timeout 
// scheduler.wait(COMPOSITOR_REDRAW_EVENT, 16 /*ms*/); // Conceptual wait 
        } 
    }
 // --- Input Event Handling (Called by Event Dispatcher) --- 
def process_input_event(event: InputEvent): void { 
// Determine target window based on event coordinates/focus 
// Convert input event (raw key/mouse) to UIEvent 
// Enqueue UIEvent onto target window's event queue 
// Signal compositor/window task if needed 
    }
 // --- Initialization --- 
@BDIOsService(id = GUI_SERVICE_ID, op = GUIOp::INIT) 
def init_gui(): bool { 
// Get screen framebuffer region from graphics driver service 
// Initialize window list, z_order 
// Register input event handlers with Event Dispatcher 
// Spawn compositor task via Scheduler service 
return true;
    }
namespace bdios::services::gui_rendering; 
import bdios::services::gui::{Rect, Color, Window}; // Use types from gui service 
import bdios::memory; // Needed if direct pointer access used 
/** 
 * @brief Fills a rectangular area within a framebuffer region with a solid color. 
 * Handles clipping to the framebuffer boundaries. 
 * Assumes RGBA or similar layout for Color. 
 */ 
def fill_rect(buffer: MemoryRegion<Color>, buffer_width: i32, rect: Rect, color: Color): void { 
// 1. Clip rectangle to buffer boundaries 
var clip_x1 = max(rect.x, 0); 
var clip_y1 = max(rect.y, 0); 
var clip_x2 = min(rect.x + rect.width, buffer_width); 
var clip_y2 = min(rect.y + rect.height, buffer.size() / (buffer_width * size_of::<Color>())); // Calculate height from size/width 
if (clip_x1 >= clip_x2 || clip_y1 >= clip_y2) { 
return; // Nothing to draw (outside bounds or zero size) 
    }
 // 2. Get direct pointer (if possible/safe) or use region access methods 
//    Using direct pointer for performance example (requires careful memory management) 
var buffer_ptr = buffer.ptr(); // Pointer<Color> 
var color_u32 = reinterpret_cast<u32>(color); // Pack color into u32 if possible for faster writes 
// 3. Iterate and write pixels 
var y = clip_y1;
 while (y < clip_y2) { 
var row_start_offset = y * buffer_width; 
var x = clip_x1; 
while (x < clip_x2) { 
var pixel_offset = row_start_offset + x; 
// Direct pointer write (assumes alignment allows u32 write) 
            store_at_ptr(buffer_ptr + pixel_offset, color_u32); // Conceptual store 
// Or write byte-by-byte if packing/alignment uncertain: 
// store_byte_at_ptr(buffer_ptr, (pixel_offset*4) + 0, color.r); 
// store_byte_at_ptr(buffer_ptr, (pixel_offset*4) + 1, color.g); 
// store_byte_at_ptr(buffer_ptr, (pixel_offset*4) + 2, color.b); 
// store_byte_at_ptr(buffer_ptr, (pixel_offset*4) + 3, color.a); 
            x = x + 1; 
        } 
        y = y + 1; 
    }
 } 
/*
 * @brief Copies (blits) a rectangular area from a source buffer to a destination buffer. 
 * Handles clipping on both source and destination. 
 * Supports basic alpha blending (source over destination). 
 */ 
def blit_rect_alpha( 
    dest_buffer: MemoryRegion<Color>, dest_width: i32, dest_rect: Rect, 
    src_buffer: MemoryRegion<Color>, src_width: i32, src_rect: Rect 
): void { 
    // 1. Calculate intersection rectangle in destination coordinates 
    var clip_x1 = max(dest_rect.x, 0); 
    var clip_y1 = max(dest_rect.y, 0); 
    var clip_x2 = min(dest_rect.x + src_rect.width, dest_width); // Use src width for blit size 
    var clip_y2 = min(dest_rect.y + src_rect.height, dest_buffer.size() / (dest_width * size_of::<Color>())); 
    if (clip_x1 >= clip_x2 || clip_y1 >= clip_y2) return; // Nothing to draw 
    var blit_width = clip_x2 - clip_x1; 
    var blit_height = clip_y2 - clip_y1; 
    // 2. Calculate corresponding source rectangle start based on clipping 
    var src_start_x = src_rect.x + (clip_x1 - dest_rect.x); 
    var src_start_y = src_rect.y + (clip_y1 - dest_rect.y); 
    // 3. Clip source rectangle start/dimensions (ensure we don't read out of bounds) 
    if (src_start_x < 0 || src_start_y < 0 || 
        src_start_x + blit_width > src_rect.x + src_rect.width || // Check against original src rect bounds 
        src_start_y + blit_height > src_rect.y + src_rect.height || 
        src_start_x + blit_width > src_width || // Check against buffer width 
        src_start_y + blit_height > src_buffer.size() / (src_width * size_of::<Color>()) ) 
    {
         print("GUI Error: Blit source bounds calculation error."); 
         return; // Invalid source region calculated 
    }
    // 4. Get pointers (or use region accessors) 
    var dest_ptr = dest_buffer.ptr(); 
    var src_ptr = src_buffer.ptr(); 
    // 5. Iterate and copy/blend pixels 
    var dy = clip_y1; 
    var sy = src_start_y; 
    while (dy < clip_y2) { 
        var dx = clip_x1;
        var sx = src_start_x; 
        while (dx < clip_x2) { 
            var dest_offset = dy * dest_width + dx; 
            var src_offset = sy * src_width + sx; 
            // Read source and destination pixels 
            var src_color: Color = load_from_ptr(src_ptr + src_offset); // Conceptual load 
            var dest_color: Color = load_from_ptr(dest_ptr + dest_offset); 
            // Perform alpha blending (Source Over) 
            // alpha = src.a / 255.0 
            // result_rgb = src.rgb * alpha + dest.rgb * (1 - alpha) 
            // result_a = src.a + dest.a * (1 - alpha) // Or simply max(src.a, dest.a)? Or opaque? 
            // --- Requires floating point math or fixed-point integer math --- 
            var alpha_norm: f32 = (src_color.a as f32) / 255.0; 
            var blended_r = ((src_color.r as f32) * alpha_norm + (dest_color.r as f32) * (1.0 - alpha_norm)) as u8; 
            var blended_g = ((src_color.g as f32) * alpha_norm + (dest_color.g as f32) * (1.0 - alpha_norm)) as u8; 
            var blended_b = ((src_color.b as f32) * alpha_norm + (dest_color.b as f32) * (1.0 - alpha_norm)) as u8; 
            var final_a = max(src_color.a, dest_color.a); // Simple alpha combine for now 
            var final_color = Color { r: blended_r, g: blended_g, b: blended_b, a: final_a }; 
            // Write blended pixel 
            store_at_ptr(dest_ptr + dest_offset, final_color); // Conceptual store 
            dx = dx + 1; 
            sx = sx + 1; 
        } 
        dy = dy + 1; 
        sy = sy + 1; 
    }
 } 
//Implement DRAW_TEXT requires: 
// 1. Font data (bitmaps or vector paths) stored in MemoryRegions. 
// 2. Font parsing logic (Chimera code -> BDI). 
// 3. Glyph rendering logic (blit individual character bitmaps/paths with color). 
// Extremely complex to do purely in software rendering via BDI ops. Usually relies on HAL/GPU. 
def draw_text(...) { /* ... Complex ... */ } 
