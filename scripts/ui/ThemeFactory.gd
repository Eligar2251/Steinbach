## ThemeFactory.gd — единая темная тема адаптивного интерфейса.
class_name ThemeFactory
extends RefCounted


## Создает стиль с рамкой и внутренними отступами.
static func box(color: Color, border: Color = Color("#344c53")) -> StyleBoxFlat:
	var s = StyleBoxFlat.new()
	s.bg_color = color
	s.border_color = border
	s.set_border_width_all(1)
	s.set_corner_radius_all(8)
	s.content_margin_left = 16
	s.content_margin_right = 16
	s.content_margin_top = 12
	s.content_margin_bottom = 12
	return s


## Генерирует тему для всех экранов без бинарных шрифтов.
static func create() -> Theme:
	var theme = Theme.new()
	theme.default_font_size = 18
	theme.set_color("font_color", "Label", Color("#e5e9df"))
	theme.set_stylebox("panel", "PanelContainer", box(Color("#14252e")))
	theme.set_stylebox("normal", "Button", box(Color("#233b43")))
	theme.set_stylebox("hover", "Button", box(Color("#35585e"), Color("#d8bc7b")))
	theme.set_stylebox("pressed", "Button", box(Color("#43696d")))
	theme.set_stylebox("disabled", "Button", box(Color("#1b2c33")))
	theme.set_color("font_disabled_color", "Button", Color("#667b7f"))
	return theme


## Кнопка с достаточно большой областью касания.
static func button(text: String, action: Callable) -> Button:
	var b = Button.new()
	b.text = text
	b.custom_minimum_size.y = 44
	b.pressed.connect(action)
	return b


## Многострочный текст.
static func label(text: String, font_size: int = 18) -> Label:
	var l = Label.new()
	l.text = text
	l.add_theme_font_size_override("font_size", font_size)
	l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	return l
