class_name WorldGenLoadScreenProgress
extends RefCounted
## Turns poll_world_gen_async snapshots into status text and timestamped log lines.


## Seconds without a stage change before each heartbeat log line (then repeats last interval).
const HEARTBEAT_INTERVALS_SEC: Array[int] = [15, 30, 45, 60, 90, 120, 180, 240, 300]

var _last_stage_id: String = ""
var _stage_started_ms: int = 0
var _heartbeat_tier: int = 0
var _last_logged_heartbeat_sec: int = 0


func reset() -> void:
	_last_stage_id = ""
	_stage_started_ms = 0
	_heartbeat_tier = 0
	_last_logged_heartbeat_sec = 0


func opening_log_lines(form_dict: Dictionary) -> PackedStringArray:
	reset()
	var lines: PackedStringArray = []
	lines.append(timestamp() + " Starting world generation…")
	lines.append(_form_summary(form_dict))
	lines.append(
		"Each pipeline step is logged below with elapsed time. "
		+ "If nothing new appears for a couple of minutes, check the last step — terrain mesh and topology are often slow on large worlds.")
	return lines


func update_from_poll(st: Dictionary) -> Dictionary:
	var out: Dictionary = {
		"status_primary": "",
		"status_detail": "",
		"log_lines": PackedStringArray(),
	}
	if not bool(st.get("running", false)):
		return out

	var stage_id: String = str(st.get("stage_id", ""))
	var stage_index: int = int(st.get("stage_index", 0))
	var stage_count: int = int(st.get("stage_count", 1))
	var progress: float = float(st.get("progress_01", 0.0))
	var elapsed_ms: int = _elapsed_on_stage_ms()

	out["status_primary"] = _status_primary(stage_id, stage_index, stage_count, progress, elapsed_ms)
	out["status_detail"] = WorldGenStageCatalog.detail_for(stage_id)

	if stage_id != _last_stage_id:
		out["log_lines"] = _log_stage_started(stage_id, stage_index, stage_count, elapsed_ms)
		_last_stage_id = stage_id
		_stage_started_ms = Time.get_ticks_msec()
		_heartbeat_tier = 0
		_last_logged_heartbeat_sec = 0
	else:
		var heartbeat: PackedStringArray = _maybe_heartbeat(stage_id, stage_index, stage_count, elapsed_ms)
		if heartbeat.size() > 0:
			out["log_lines"] = heartbeat

	return out


static func format_elapsed(ms: int) -> String:
	var total_sec: int = maxi(ms / 1000, 0)
	if total_sec < 60:
		return "%ds" % total_sec
	var minutes: int = total_sec / 60
	var seconds: int = total_sec % 60
	if minutes < 60:
		return "%dm %02ds" % [minutes, seconds]
	var hours: int = minutes / 60
	minutes = minutes % 60
	return "%dh %02dm %02ds" % [hours, minutes, seconds]


func _elapsed_on_stage_ms() -> int:
	if _stage_started_ms <= 0:
		return 0
	return Time.get_ticks_msec() - _stage_started_ms


func _status_primary(
	stage_id: String,
	stage_index: int,
	stage_count: int,
	progress: float,
	elapsed_ms: int
) -> String:
	var step: String = WorldGenStageCatalog.step_label(stage_index, stage_count)
	var title: String = WorldGenStageCatalog.title_for(stage_id)
	var pct: int = clampi(int(round(progress * 100.0)), 0, 100)
	var elapsed: String = format_elapsed(elapsed_ms)
	if stage_id.is_empty():
		return "%s · Initialising… (%s)" % [step, elapsed]
	return "%s · %s — %d%% (%s on this step)" % [step, title, pct, elapsed]


func _log_stage_started(
	stage_id: String,
	stage_index: int,
	stage_count: int,
	elapsed_ms: int
) -> PackedStringArray:
	var lines: PackedStringArray = []
	var step: String = WorldGenStageCatalog.step_label(stage_index, stage_count)
	var title: String = WorldGenStageCatalog.title_for(stage_id)
	var detail: String = WorldGenStageCatalog.detail_for(stage_id)
	var id_suffix: String = "" if stage_id.is_empty() else " [%s]" % stage_id
	lines.append(timestamp() + " %s · %s%s" % [step, title, id_suffix])
	if not detail.is_empty():
		lines.append("    " + detail)
	if elapsed_ms > 0 and not stage_id.is_empty():
		lines.append("    (previous step took %s)" % format_elapsed(elapsed_ms))
	return lines


func _maybe_heartbeat(stage_id: String, stage_index: int, stage_count: int, elapsed_ms: int) -> PackedStringArray:
	var elapsed_sec: int = elapsed_ms / 1000
	if _heartbeat_tier >= HEARTBEAT_INTERVALS_SEC.size():
		var repeat_interval: int = HEARTBEAT_INTERVALS_SEC[HEARTBEAT_INTERVALS_SEC.size() - 1]
		if elapsed_sec - _last_logged_heartbeat_sec < repeat_interval:
			return PackedStringArray()
		_last_logged_heartbeat_sec = elapsed_sec
	else:
		var threshold: int = HEARTBEAT_INTERVALS_SEC[_heartbeat_tier]
		if elapsed_sec < threshold:
			return PackedStringArray()
		_last_logged_heartbeat_sec = elapsed_sec
		_heartbeat_tier += 1

	var lines: PackedStringArray = []
	var step: String = WorldGenStageCatalog.step_label(stage_index, stage_count)
	var title: String = WorldGenStageCatalog.title_for(stage_id)
	var id_part: String = "" if stage_id.is_empty() else " [%s]" % stage_id
	lines.append(
		timestamp()
		+ " Still on %s · %s%s — %s elapsed on this step (no step change yet)."
		% [step, title, id_part, format_elapsed(elapsed_ms)])
	var detail: String = WorldGenStageCatalog.detail_for(stage_id)
	if not detail.is_empty():
		lines.append("    " + detail)
	if stage_id == "terrain_mesh" or stage_id == "topology":
		lines.append("    Large site counts can keep this step busy for several minutes; the sim is still working.")
	return lines


static func _form_summary(form_dict: Dictionary) -> String:
	var preset: String = str(form_dict.get("planet_preset", "?"))
	var sites: int = int(form_dict.get("voronoi_sites", 0))
	var mode: String = str(form_dict.get("world_gen_mode", "?"))
	var mesh: bool = bool(form_dict.get("use_planet_terrain_mesh", false))
	var simd: bool = bool(form_dict.get("use_simd", false))
	var sim_sites = form_dict.get("sim_voronoi_sites", null)
	var sim_note: String = ""
	if sim_sites != null:
		sim_note = " · coarse sim sites=%s" % str(sim_sites)
	return (
		"Preset=%s · voronoi_sites=%d%s · mode=%s · terrain_mesh=%s · simd=%s"
		% [preset, sites, sim_note, mode, mesh, simd])


static func timestamp() -> String:
	var dt := Time.get_datetime_dict_from_system()
	return "[%02d:%02d:%02d]" % [dt.hour, dt.minute, dt.second]
