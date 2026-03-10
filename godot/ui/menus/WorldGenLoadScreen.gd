extends Control
## Load screen shown between Generate and world display. Shows progress and logs; on Continue, emits generation_finished(result).

signal generation_finished(result: Dictionary)

var _form_dict: Dictionary
var _result: Dictionary

@onready var _title_label: Label = $MarginContainer/CenterContainer/Card/CardPadding/VBox/TitleLabel
@onready var _progress_bar: ProgressBar = $MarginContainer/CenterContainer/Card/CardPadding/VBox/ProgressBar
@onready var _log_scroll: ScrollContainer = $MarginContainer/CenterContainer/Card/CardPadding/VBox/LogScroll
@onready var _log_label: RichTextLabel = $MarginContainer/CenterContainer/Card/CardPadding/VBox/LogScroll/LogLabel
@onready var _continue_btn: Button = $MarginContainer/CenterContainer/Card/CardPadding/VBox/ContinueBtn


func _ready() -> void:
	_continue_btn.visible = false
	_continue_btn.pressed.connect(_on_continue_pressed)
	_log_label.clear()
	_log_label.add_text("Preparing...\n")


## Call this after the node is in the tree. Runs world gen on the next frame so the load screen is visible first.
func run_generation(form_dict: Dictionary) -> void:
	_form_dict = form_dict
	_title_label.text = "Generating world..."
	_progress_bar.value = 0
	_progress_bar.show_percentage = true
	call_deferred("_do_run_generation")


func _do_run_generation() -> void:
	_result = SimAPI.apply_world_gen_form(_form_dict)
	_on_generation_done(_result)


func _on_generation_done(result: Dictionary) -> void:
	_progress_bar.value = 100
	_title_label.text = "World ready"
	var log_lines = result.get("log_lines", null)
	if log_lines is Array and log_lines.size() > 0:
		_log_label.clear()
		for line in log_lines:
			var s = str(line) if line != null else ""
			_log_label.add_text(s + "\n")
		# Scroll to bottom (next frame so scroll bar is updated)
		await get_tree().process_frame
		var v_bar = _log_scroll.get_v_scroll_bar()
		if v_bar:
			v_bar.value = v_bar.max_value
	else:
		_log_label.add_text("\n(No log lines returned)\n")
	_continue_btn.visible = true


func _on_continue_pressed() -> void:
	generation_finished.emit(_result)
	queue_free()
