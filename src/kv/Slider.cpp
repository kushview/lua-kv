/// Slider widget.
// Is a @{kv.Widget}
// @classmod kv.Slider
// @pragma nostrip

#include "kv/lua/object.hpp"
#include "kv/lua/widget.hpp"

#define LKV_TYPE_NAME_SLIDER "Slider"

namespace kv {
namespace lua {

class Slider : public juce::Slider
{
public:
    Slider (const sol::table&)
        : juce::Slider() {}
    ~Slider() {}

    static void init (const sol::table& proxy)
    {
        if (auto* impl = object_userdata<Slider> (proxy))
        {
            impl->proxy = proxy;
            impl->initialize();
        }
    }

    void initialize()
    {
        /// Handlers.
        // @section handlers

        /// Value changed.
        // @tfield function Slider.valuechanged
        onValueChange = [this]()
        {
            if (sol::function f = proxy ["valuechanged"])
            {
                auto r = f (proxy);
                if (! r.valid())
                {
                    sol::error e = r;
                    DBG(e.what());
                }
            }
        };

        /// Started to drag.
        // @tfield function Slider.dragstart
        onDragStart = [this]() {
            if (sol::function f = proxy ["dragstart"]) {
                f (proxy);
            }
        };

        /// Stopped dragging.
        // @tfield function Slider.dragend
        onDragEnd = [this]() {
            if (sol::function f = proxy ["dragend"]) {
                f (proxy);
            }
        };
    }

private:
    sol::table proxy;
};

}}

LUAMOD_API
int luaopen_kv_Slider (lua_State* L) {
    using kv::lua::Slider;

    auto T = kv::lua::new_widgettype<Slider> (L, LKV_TYPE_NAME_SLIDER,
        sol::meta_method::to_string, [](Slider& self) {
            return kv::lua::to_string (self, LKV_TYPE_NAME_SLIDER);
        },

        /// Attributes.
        // @section attributes

        /// Minimum range (readonly).
        // @see Slider.range
        // @tfield number Slider.min
        "min", sol::readonly_property (&Slider::getMinimum),

        /// Maximum range (readonly).
        // @see Slider.range
        // @tfield number Slider.max
        "max", sol::readonly_property (&Slider::getMaximum),

        /// Current step-size (readonly).
        // @see Slider.range
        // @tfield number Slider.interval
        "interval", sol::readonly_property (&Slider::getInterval),

        /// Kind of slider.
        // @tfield int Slider.style
        "style", sol::property (
            [](Slider& self) -> int { return static_cast<int> (self.getSliderStyle()); },
            [](Slider& self, int style) -> void {
                if (! isPositiveAndBelow (style, Slider::ThreeValueVertical))
                    style = Slider::LinearHorizontal;
                self.setSliderStyle (static_cast<Slider::SliderStyle> (style));
            }
        ),


        /// Methods.
        // @section methods

        /// Get the current range.
        // @function Slider:range
        // @return kv.Range The current range

        /// Set the range.
        // @function Slider:range
        // @number min
        // @number max
        // @number interval (default: 0.0)
        // @return kv.Range The current range

        /// Set the range.
        // @function Slider:range
        // @tparam kv.Range range Range to set
        // @number interval Step-size
        // @return kv.Range The current range
        "range", sol::overload (
            [](Slider& self) {
                return self.getRange();
            },
            [](Slider& self, double min, double max) {
                self.setRange (min, max);
                return self.getRange();
            },
            [](Slider& self, double min, double max, double interval) {
                self.setRange (min, max, interval);
                return self.getRange();
            },
            [](Slider& self, Range<double> r, double interval) {
                self.setRange (r, interval);
                return self.getRange();
            }
        ),

        /// Get or set the value.
        // @function Slider:value
        "value", sol::overload (
            [](Slider& self) { return self.getValue(); },
            [](Slider& self, double value) {
                self.setValue (value, juce::sendNotificationAsync);
                return self.getValue();
            },
            [](Slider& self, double value, int notify) {
                if (! isPositiveAndBelow (notify, 4))
                    notify = sendNotificationAsync;
                self.setValue (value, static_cast<NotificationType> (notify));
                return self.getValue();
            }
        ),

        /// Change TextBox position.
        // @function Slider:textboxstyle
        // @int pos Text box position
        // @bool ro Text box is read only
        // @int width Text box width
        // @int height Text box height
        "textboxstyle", &Slider::setTextBoxStyle,

        sol::base_classes, sol::bases<juce::Component>()
    );

    /// Styles. 
    // @section styles

    /// Linear Horizontal.
    // @tfield int Slider.LINEAR_HORIZONTAL
    T["LINEAR_HORIZONTAL"]      = (lua_Integer) Slider::LinearHorizontal;

    /// Linear Vertical.
    // @tfield int Slider.LINEAR_VERTICAL
    T["LINEAR_VERTICAL"]        = (lua_Integer) Slider::LinearVertical;

    /// Linear Bar.
    // @tfield int Slider.LINEAR_BAR
    T["LINEAR_BAR"]             = (lua_Integer) Slider::LinearBar;

    /// Linear Bar Vertical.
    // @tfield int Slider.LINEAR_BAR_VERTICAL
    T["LINEAR_BAR_VERTICAL"]    = (lua_Integer) Slider::LinearBarVertical;

    /// Rotary.
    // @tfield int Slider.ROTARY
    T["ROTARY"]                 = (lua_Integer) Slider::Rotary;

    /// Rotary horizontal drag.
    // @tfield int Slider.ROTARY_HORIZONTAL_DRAG
    T["ROTARY_HORIZONTAL_DRAG"] = (lua_Integer) Slider::RotaryHorizontalDrag;
    
    /// Rotary vertical drag.
    // @tfield int Slider.ROTARY_VERTICAL_DRAG
    T["ROTARY_VERTICAL_DRAG"]   = (lua_Integer) Slider::RotaryVerticalDrag;

    /// Rotary horizontal/vertical drag.
    // @tfield int Slider.ROTARY_HORIZONTAL_VERTICAL_DRAG
    T["ROTARY_HORIZONTAL_VERTICAL_DRAG"] = (lua_Integer) Slider::RotaryHorizontalVerticalDrag;

    /// Spin buttons.
    // @tfield int Slider.SPIN_BUTTONS
    T["SPIN_BUTTONS"]        = (lua_Integer) Slider::IncDecButtons;

    /// Two value horizontal.
    // @tfield int Slider.TWO_VALUE_HORIZONTAL
    T["TWO_VALUE_HORIZONTAL"]   = (lua_Integer) Slider::TwoValueHorizontal;

    /// Two value vertical.
    // @tfield int Slider.TWO_VALUE_VERTICAL
    T["TWO_VALUE_VERTICAL"]     = (lua_Integer) Slider::TwoValueVertical;

    /// Three value horizontal.
    // @tfield int Slider.THREE_VALUE_HORIZONTAL
    T["THREE_VALUE_HORIZONTAL"] = (lua_Integer) Slider::ThreeValueHorizontal;

    /// Three value vertical.
    // @tfield int Slider.THREE_VALUE_VERTICAL
    T["THREE_VALUE_VERTICAL"]   = (lua_Integer) Slider::ThreeValueVertical;
    
    /// TextBox Position. 
    // @section textbox

    /// No TextBox.
    // @tfield int Slider.TEXT_BOX_NONE
    T["TEXT_BOX_NONE"]          = (lua_Integer) Slider::NoTextBox;

    /// TextBox on left.
    // @tfield int Slider.TEXT_BOX_LEFT
    T["TEXT_BOX_LEFT"]          = (lua_Integer) Slider::TextBoxLeft;

    /// TextBox on right.
    // @tfield int Slider.TEXT_BOX_RIGHT
    T["TEXT_BOX_RIGHT"]         = (lua_Integer) Slider::TextBoxRight;

    /// TextBox above.
    // @tfield int Slider.TEXT_BOX_ABOVE
    T["TEXT_BOX_ABOVE"]         = (lua_Integer) Slider::TextBoxAbove;

    /// TextBox below.
    // @tfield int Slider.TEXT_BOX_BELOW
    T["TEXT_BOX_BELOW"]         = (lua_Integer) Slider::TextBoxBelow;
    

    /// Drag Mode. 
    // @section textbox
    
    /// Not dragging.
    // @tfield int Slider.DRAG_NONE
    T["DRAG_NONE"]              = (lua_Integer) Slider::notDragging;
    
    /// Absolute dragging.
    // @tfield int Slider.DRAG_ABSOLUTE
    T["DRAG_ABSOLUTE"]          = (lua_Integer) Slider::absoluteDrag;

    /// Velocity based dragging.
    // @tfield int Slider.DRAG_VELOCITY
    T["DRAG_VELOCITY"]          = (lua_Integer) Slider::velocityDrag;

    sol::table T_mt = T[sol::metatable_key];
    T_mt["__props"].get_or_create<sol::table>().add (
        "min", "max", "interval", "style"
    );
    T_mt["__methods"].get_or_create<sol::table>().add (
        "range", "value", "textboxstyle"
    );

    sol::stack::push (L, T);
    return 1;
}