# UML funcional: `backend/python`

Funciones detectadas: **193**. Tipos detectados: **19**.

## Grafo de llamadas

```mermaid
flowchart LR
  subgraph f0["app.py"]
    n4303201550["_load_application()"]
    nd5db088795["parse_args()"]
    n9222a02d25["main()"]
    n3e89fea1ba["shutdown_host()"]
  end
  subgraph f1["__init__.py"]
    nfb667eae25["create_app(*args: Any, **kwargs: Any)"]
  end
  subgraph f2["app_factory.py"]
    nfb676358e5["create_app(config: AppConfig | None = None)"]
    nf6fc8bbb4a["create_csp_nonce()"]
    nc01be743ca["secure_headers(response: Any)"]
  end
  subgraph f3["compaction.py"]
    n8d63ce830a["_quantize(value: Any, quantum: float)"]
    n332e428c8f["_normalized_payload(raw: str)"]
    n91f6dd4e52["clean(value: Any)"]
    nc654c3b31c["_telemetry_key(row: sqlite3.Row)"]
    n9ca71c4ce2["compact_database(path: Path, apply: bool = False, vacuum: bool = False)"]
  end
  subgraph f4["config.py"]
    n9dad8b7313["normalize_robot_host(value: object)"]
    n6793031b47["from_environment(cls)"]
  end
  subgraph f5["database.py"]
    n720b5e0530["__init__(self, path: Path, clock: Callable[[], datetime] | None = None)"]
    n1dd42260be["_session_timestamp(self)"]
    n81797d6e71["connect(self)"]
    n50a87bcbf6["transaction(self)"]
    n37f1acf921["initialize(self)"]
    ndf873e457e["get_setting(self, key: str, default: Any = None)"]
    n91caf2e441["set_setting(self, key: str, value: Any)"]
    n9eb1fa88dd["create_session(self)"]
    n6fd7dae001["update_session_identity(self, session_id: int, robot_id: str | None, firmware_version: str | None, protocol: str | None)"]
    n918723df5e["stop_session(self, session_id: int, reason: str | None = None)"]
    n2c767e452a["update_session_disconnect_reason(self, session_id: int, reason: str | None)"]
    nfa0fd60de8["insert_command(self, command_id: str, session_id: int | None, name: str, payload: dict[str, Any], status: str)"]
    nea7899285e["update_command(self, command_id: str, status: str, error: str | None = None)"]
    nda6b3ac5df["fail_nonterminal_commands(self, reason: str)"]
    na20c316ef7["close_orphan_sessions(self, reason: str)"]
    n3aec5e6c50["insert_event(self, session_id: int | None, kind: str, severity: str, payload: dict[str, Any])"]
    n40030491a7["_quantize(value: Any, quantum: float)"]
    n1adac0dfef["telemetry_priority_signature(cls, snapshot: Any)"]
    n26913ceebe["telemetry_fingerprint(cls, snapshot: Any)"]
    n479954f0d2["_compact_payload(snapshot: Any)"]
    nd1e827a85e["insert_telemetry(self, session_id: int, snapshot: Any)"]
    nf84cac3046["telemetry_rows(self, session_id: int)"]
    n85f9e2f6af["session_rows(self)"]
    n4c43215331["session_row(self, session_id: int)"]
    n97dd046d7f["command_rows(self, session_id: int)"]
    n678ac9d473["event_rows(self, session_id: int)"]
    n6c1dffcb0a["purge_sessions(self, days: int)"]
    n6495b08fb4["save_touch_recording(self, recording: dict[str, Any])"]
    n22e4a8e8ff["touch_recordings(self)"]
    n69e54add6a["touch_recording(self, recording_id: str)"]
    n46184faee8["delete_touch_recording(self, recording_id: str)"]
    n8fec4a08ed["acknowledge_touch_recording(self, recording_id: str)"]
    n60d788c38a["invalidate_open_touch_recordings(self, reason: str)"]
    n498dc74939["optimize_storage(self, full: bool = False)"]
    nbf9263d294["list_calibration_surfaces(self)"]
    n7c3e14c330["get_calibration_surface(self, surface_id: str)"]
    nab5bbc4af8["save_calibration_surface( self, surface_id: str, name: str, pwm_positive_8bit: int, pwm_negative_8bit: int, positive_polarity: int, negative_polarity: int, description: str | None = None, trim_izq: float = 1.0, trim_der: float = 1.0, deadband_izq_8bit: int = 0, deadband_der_8bit: int = 0, icr_x_cm: float = 0.0, icr_y_cm: float = 0.0, gyro_scale: float = 1.0, )"]
    n2ea2a910cf["delete_calibration_surface(self, surface_id: str)"]
  end
  subgraph f6["domain.py"]
    nac1e082ebf["_finite_number(value: Any, name: str, low: float, high: float)"]
    n79b8e2b7e4["_heading_degrees(value: Any)"]
    nb844e37ed3["validate_command_payload(name: str, payload: Mapping[str, Any] | None)"]
    nc51207575a["create(cls, name: object, payload: Mapping[str, Any] | None = None, seq: int = 0, command_id: str | None = None)"]
    n6f46fd19a8["protocol_envelope(self)"]
    n84e1213c58["split_segment_mm( start_x_mm: float, start_y_mm: float, end_x_mm: float, end_y_mm: float, max_segment_mm: float = MAX_SEGMENT_MM, )"]
    nd5915cad45["from_message(cls, message: Mapping[str, Any], fallback_sequence: int)"]
    n35c6ea2d59["public_dict(self)"]
  end
  subgraph f7["gateway.py"]
    n9db805ee4f["__init__( self, host_getter: Callable[[], str], on_message: Callable[[dict[str, Any]], None], on_state: Callable[[ConnectionState, str | None], None], on_sent: Callable[[RobotCommand], None], session_getter: Callable[[], str] = lambda: '', max_message_bytes: int = MAX_ROBOT_MESSAGE_BYTES, )"]
    n5cc81c5e47["start(self)"]
    n0ca5a9c07f["stop(self)"]
    nea93ccbc40["request_reconnect(self)"]
    neecb007ecd["enqueue(self, command: RobotCommand)"]
    n211ac890ff["cancel(self, command_id: str | None)"]
    nd997de33d8["snapshot(self)"]
    n5d15be8f9a["_set_state(self, state: ConnectionState, detail: str | None)"]
    n374a8c751b["_run(self)"]
    ne6580eac0b["_drain_one(self, connection: Any)"]
    n77c69d98ee["_receive(self, raw: str | bytes)"]
  end
  subgraph f8["route_planning.py"]
    n719a3019d3["_finite_number(value: object, name: str, low: float, high: float)"]
    n9daf052028["normalize_signed_degrees(value: float)"]
    na6412ea892["compile(self, request: Mapping[str, Any], origin: Mapping[str, float])"]
    nb4a1e74cf7["_validated_origin(origin: Mapping[str, float])"]
    nba74d7f0f8["_finish(mode: str, logical_steps: list[dict[str, Any]], segments: list[dict[str, Any]])"]
    n6dbe6a9578["_append_segment( segments: list[dict[str, Any]], start_x: float, start_y: float, target_x: float, target_y: float, logical_step_id: int, component: str, )"]
    n78a8b9aa9e["compile(self, request: Mapping[str, Any], origin: Mapping[str, float])"]
    n0e1fa47dee["compile(self, request: Mapping[str, Any], origin: Mapping[str, float])"]
    n28d1dcf52b["strategy_for_request(request: Mapping[str, Any])"]
    n5c0fbdc43d["simplify_rdp(points: list[dict[str, float]], tolerance_mm: float = 20.0)"]
    n3e673f6e4a["distance(point: dict[str, float], start: dict[str, float], end: dict[str, float])"]
    n92c3726b2a["compile_orthogonal_points(points: list[dict[str, float]], max_segment_mm: float = DEFAULT_SUBSEGMENT_MM, drive_mode: str = 'auto', deduct_chassis_offset: bool = False)"]
    n3863aa4d2b["compile_touch_path(points: list[dict[str, float]])"]
  end
  subgraph f9["services.py"]
    nfa2e0f121a["__init__(self, subscriber_size: int = 64)"]
    nfe20eab7ab["subscribe(self)"]
    nb44dcd1833["unsubscribe(self, subscriber: queue.Queue[str])"]
    nac21e37a2a["publish(self, kind: str, payload: dict[str, Any])"]
    n87eeda9d0a["__init__(self, database: Database, queue_size: int = 256)"]
    n14396af8ec["submit(self, session_id: int, snapshot: TelemetrySnapshot)"]
    n91340c1852["stop(self)"]
    ne0599e1541["_run(self)"]
    nbd38f1954c["__init__(self, database: Database)"]
    n9c5f83b7c9["start(self, snapshot: TelemetrySnapshot)"]
    n5dd3a6928b["add(self, snapshot: TelemetrySnapshot, force: bool = False)"]
    n9005492f16["invalidate(self, reason: str)"]
    n89d52f0023["finish(self, reason: str = 'manual_stop', invalid: bool = False)"]
    nb93c2d043c["public(self)"]
    n90d37ece3c["summary(self)"]
    naa4f1bd8ff["__init__(self, database: Database, host: str | None = None, start_gateway: bool = True, max_message_bytes: int = 4096, settling_delay_s: float = 0.0)"]
    n8a55774203["close(self)"]
    na58c542b94["get_robot_host(self)"]
    ndba31c55ba["set_robot_host(self, value: object)"]
    n576b15c811["robot_config(self)"]
    n96cf03ffa9["set_vectorial_routes_enabled(self, value: object)"]
    n1609e58b46["connect(self)"]
    ne57b2b2018["disconnect(self)"]
    n226e0c2b69["reconnect(self)"]
    nb74aadb041["status(self)"]
    na4e1f5b2a8["mission_status(self)"]
    n574112e035["_persist_mission(self)"]
    nc8bc2a0782["_mission_wire_points(self)"]
    n054886fd9b["_require_ready_robot(self)"]
    nd0b1b13b33["_require_vectorial_ready(self, telemetry: TelemetrySnapshot | None = None)"]
    n93eb1479e9["_build_segments(self, points: list[Any], mission_origin: dict[str, float])"]
    n06f5cd7180["_activate_mission(self, segments: list[dict[str, Any]], mission_origin: dict[str, float], kind: str, final_heading: float | None = None, mode: str = 'rectangular', logical_steps: list[dict[str, Any]] | None = None)"]
    n91e2659598["start_mission(self, request: object)"]
    n9434a0b72a["manual_start(self)"]
    nddc71c4bc6["manual_drive(self, payload: object)"]
    nd922b8146c["manual_stop(self)"]
    n6a0e117aa8["_touch_row(row: Any, include_samples: bool = True)"]
    ndcccba356c["touch_recordings(self)"]
    n5c439ba5fa["touch_recording(self, recording_id: str)"]
    n2c90211809["acknowledge_open_area(self, recording_id: str)"]
    n8cef5211ce["return_touch_recording(self, recording_id: str)"]
    n159be3f1e1["start_return_home(self, preview: bool = False)"]
    n47db3f7310["stop_mission(self, reason: str = 'operator_stop')"]
    n926494aa19["clear_robot_mission_memory(self)"]
    n941209c201["_advance_mission(self)"]
    nd1d5bda3d6["_set_return_state(self, state: str, error: str | None = None)"]
    n20e1d4f1bd["_queue_current_mission_step(self, seq_override: int | None = None)"]
    n721cae00c8["_queue_final_alignment(self, seq_override: int | None = None)"]
    n8e2b4347e1["_expire_stalled_mission(self)"]
    nf2ae7e0680["_block_mission(self, reason: str)"]
    n0a7c80fcad["_reconcile_mission_snapshot(self, snapshot: TelemetrySnapshot)"]
    n5fd3ee00e7["_reconcile_firmware_mission(self, firmware: dict[str, Any])"]
    nd41c7b3338["send_command(self, name: object, payload: dict[str, Any] | None, seq_override: int | None = None, command_id_override: str | None = None)"]
    n4885a4566a["start_session(self)"]
    nca72b657ff["stop_session(self, reason: str | None = None)"]
    n264336e253["_cancel_disconnect_grace_timer(self)"]
    nd263ebfd5e["_schedule_disconnect_grace(self, detail: str)"]
    n86083e5721["_on_disconnect_grace_expired(self, detail: str)"]
    n3484b80fe4["_on_connection_state(self, state: ConnectionState, detail: str | None)"]
    nca0045ec9e["_on_command_sent(self, command: RobotCommand)"]
    n05fa14e75a["_on_robot_message(self, message: dict[str, Any])"]
    n6da73c81ea["_on_step_settled(self)"]
    n3d50adb5e0["_reconcile_short_memory_hello(self, message: dict[str, Any])"]
    n5ca21e0784["_movement_requires_stop(self)"]
    n8339df6195["_archive_mission_for_close(self)"]
    nf3ee2fe442["_finish_close_preparation(self, force: bool, stop_required: bool, stop_confirmed: bool)"]
    nc96a78182d["prepare_close(self, force: bool = False, timeout_s: float | None = None)"]
    ncfcaf08821["purge_sessions(self, days: int)"]
    n00241b2850["_format_surface_dict(self, row: Any)"]
    nfbd041d4ef["list_calibration_surfaces(self)"]
    n2b2d1dce7b["get_calibration_surface(self, surface_id: str)"]
    n27c57d90b7["save_calibration_surface( self, name: str, pwm_pos: int, pwm_neg: int, cand_pos: int, cand_neg: int, description: str | None = None, surface_id: str | None = None, trim_izq: float = 1.0, trim_der: float = 1.0, deadband_izq_8bit: int = 0, deadband_der_8bit: int = 0, icr_x_cm: float = 0.0, icr_y_cm: float = 0.0, gyro_scale: float = 1.0, )"]
    nc990202d07["delete_calibration_surface(self, surface_id: str)"]
    nabe7085c3c["apply_calibration_surface(self, surface_id: str)"]
    n7d83a20d1d["_sync_surface_torque_from_telemetry(self, base_pos: int, base_neg: int)"]
  end
  subgraph f10["web.py"]
    n11a7fd3b87["_service()"]
    n9768753665["_require_token(function: Callable[..., Any])"]
    nab39be2bb6["wrapper(*args: Any, **kwargs: Any)"]
    nef186c8e4b["index()"]
    nda183cb233["ui_asset(filename: str)"]
    n815823909d["status()"]
    n1eb416b959["robot_config()"]
    n0cf4ca5f46["connection(action: str)"]
    n33765df17c["command()"]
    ne105c688d8["estop()"]
    n36fb8ff079["manual_start()"]
    n9581512a9e["manual_drive()"]
    n5b56b0b4a6["manual_stop()"]
    n99b1f18848["touch_recordings()"]
    n0ae797aea0["touch_recording(recording_id: str)"]
    ncfb946487b["delete_touch_recording(recording_id: str)"]
    n88199824c6["acknowledge_open_area(recording_id: str)"]
    n5ac028934f["return_touch_recording(recording_id: str)"]
    nfa70e41c0f["missions()"]
    n2cf25eaa6f["clear_robot_mission_memory()"]
    ne9f0365e4c["return_home()"]
    n8840e2b858["close_application()"]
    na228d598b6["start_session()"]
    nc0c8ddce55["stop_session()"]
    n024799fea3["cleanup_sessions()"]
    nde71a3f98d["calibration_surfaces()"]
    nfc5c76ccd9["calibration_surface_item(surface_id: str)"]
    n0bec3f253d["apply_calibration_surface(surface_id: str)"]
    ndd0a0247c8["export_telemetry(session_id: int)"]
    n531233f1e6["sessions()"]
    n7bd724cf63["export_session_json(session_id: int)"]
    ne5be340822["events()"]
    nd4a1b8d0ef["stream()"]
  end
  n024799fea3 --> n11a7fd3b87
  n024799fea3 --> n6c1dffcb0a
  n054886fd9b --> nd997de33d8
  n05fa14e75a --> n14396af8ec
  n05fa14e75a --> n1adac0dfef
  n05fa14e75a --> n211ac890ff
  n05fa14e75a --> n35c6ea2d59
  n05fa14e75a --> n3aec5e6c50
  n05fa14e75a --> n3d50adb5e0
  n05fa14e75a --> n47db3f7310
  n05fa14e75a --> n4885a4566a
  n05fa14e75a --> n574112e035
  n05fa14e75a --> n5cc81c5e47
  n05fa14e75a --> n5dd3a6928b
  n05fa14e75a --> n6da73c81ea
  n05fa14e75a --> n6fd7dae001
  n05fa14e75a --> n7d83a20d1d
  n05fa14e75a --> n8e2b4347e1
  n05fa14e75a --> n9005492f16
  n05fa14e75a --> n90d37ece3c
  n05fa14e75a --> n918723df5e
  n05fa14e75a --> n941209c201
  n05fa14e75a --> na4e1f5b2a8
  n05fa14e75a --> nac21e37a2a
  n05fa14e75a --> nd41c7b3338
  n05fa14e75a --> nd5915cad45
  n05fa14e75a --> nd997de33d8
  n05fa14e75a --> nea7899285e
  n05fa14e75a --> nf2ae7e0680
  n06f5cd7180 --> n20e1d4f1bd
  n06f5cd7180 --> n211ac890ff
  n06f5cd7180 --> n574112e035
  n06f5cd7180 --> n91caf2e441
  n06f5cd7180 --> na4e1f5b2a8
  n06f5cd7180 --> nac21e37a2a
  n06f5cd7180 --> ndf873e457e
  n0a7c80fcad --> n5fd3ee00e7
  n0a7c80fcad --> nf2ae7e0680
  n0ae797aea0 --> n11a7fd3b87
  n0bec3f253d --> n11a7fd3b87
  n0ca5a9c07f --> n5d15be8f9a
  n0cf4ca5f46 --> n11a7fd3b87
  n0cf4ca5f46 --> nb74aadb041
  n0e1fa47dee --> n6dbe6a9578
  n0e1fa47dee --> n9daf052028
  n0e1fa47dee --> nac1e082ebf
  n0e1fa47dee --> nb4a1e74cf7
  n0e1fa47dee --> nba74d7f0f8
  n0e1fa47dee --> nc51207575a
  n159be3f1e1 --> n054886fd9b
  n159be3f1e1 --> n06f5cd7180
  n159be3f1e1 --> n91caf2e441
  n159be3f1e1 --> nd0b1b13b33
  n159be3f1e1 --> ndf873e457e
  n1609e58b46 --> n5cc81c5e47
  n1eb416b959 --> n11a7fd3b87
  n1eb416b959 --> n96cf03ffa9
  n1eb416b959 --> ndba31c55ba
  n20e1d4f1bd --> n574112e035
  n20e1d4f1bd --> nd0b1b13b33
  n20e1d4f1bd --> nd41c7b3338
  n20e1d4f1bd --> nf2ae7e0680
  n211ac890ff --> n5dd3a6928b
  n226e0c2b69 --> n5cc81c5e47
  n226e0c2b69 --> nd997de33d8
  n226e0c2b69 --> nea93ccbc40
  n22e4a8e8ff --> n81797d6e71
  n264336e253 --> n211ac890ff
  n26913ceebe --> n1adac0dfef
  n26913ceebe --> n8d63ce830a
  n27c57d90b7 --> n00241b2850
  n27c57d90b7 --> n7c3e14c330
  n27c57d90b7 --> nac21e37a2a
  n2b2d1dce7b --> n00241b2850
  n2c767e452a --> n50a87bcbf6
  n2c90211809 --> n69e54add6a
  n2c90211809 --> n8fec4a08ed
  n2cf25eaa6f --> n11a7fd3b87
  n2ea2a910cf --> n50a87bcbf6
  n332e428c8f --> n91f6dd4e52
  n33765df17c --> n11a7fd3b87
  n33765df17c --> nd41c7b3338
  n3484b80fe4 --> n264336e253
  n3484b80fe4 --> n2c767e452a
  n3484b80fe4 --> n3aec5e6c50
  n3484b80fe4 --> n9005492f16
  n3484b80fe4 --> n918723df5e
  n3484b80fe4 --> nac21e37a2a
  n3484b80fe4 --> nd263ebfd5e
  n36fb8ff079 --> n11a7fd3b87
  n374a8c751b --> n5d15be8f9a
  n374a8c751b --> n77c69d98ee
  n374a8c751b --> n8a55774203
  n374a8c751b --> ne6580eac0b
  n37f1acf921 --> n22e4a8e8ff
  n37f1acf921 --> n81797d6e71
  n37f1acf921 --> n8a55774203
  n37f1acf921 --> nde71a3f98d
  n3863aa4d2b --> n5c0fbdc43d
  n3863aa4d2b --> n92c3726b2a
  n3aec5e6c50 --> n50a87bcbf6
  n3aec5e6c50 --> ne5be340822
  n3d50adb5e0 --> n20e1d4f1bd
  n3d50adb5e0 --> n3aec5e6c50
  n3d50adb5e0 --> n721cae00c8
  n3d50adb5e0 --> n941209c201
  n3d50adb5e0 --> nd41c7b3338
  n3d50adb5e0 --> nea7899285e
  n3d50adb5e0 --> nf2ae7e0680
  n3e89fea1ba --> n8a55774203
  n46184faee8 --> n50a87bcbf6
  n47db3f7310 --> n211ac890ff
  n47db3f7310 --> n574112e035
  n47db3f7310 --> n91caf2e441
  n47db3f7310 --> na4e1f5b2a8
  n47db3f7310 --> nac21e37a2a
  n47db3f7310 --> nd1d5bda3d6
  n47db3f7310 --> nd41c7b3338
  n47db3f7310 --> ndf873e457e
  n47db3f7310 --> nea7899285e
  n4885a4566a --> n9eb1fa88dd
  n4885a4566a --> nac21e37a2a
  n498dc74939 --> n81797d6e71
  n498dc74939 --> n8a55774203
  n4c43215331 --> n81797d6e71
  n50a87bcbf6 --> n81797d6e71
  n50a87bcbf6 --> n8a55774203
  n531233f1e6 --> n11a7fd3b87
  n531233f1e6 --> n85f9e2f6af
  n574112e035 --> n91caf2e441
  n5ac028934f --> n11a7fd3b87
  n5b56b0b4a6 --> n11a7fd3b87
  n5c0fbdc43d --> n3e673f6e4a
  n5c0fbdc43d --> n5dd3a6928b
  n5c439ba5fa --> n6a0e117aa8
  n5cc81c5e47 --> nd997de33d8
  n5dd3a6928b --> n89d52f0023
  n5fd3ee00e7 --> n574112e035
  n5fd3ee00e7 --> na4e1f5b2a8
  n5fd3ee00e7 --> nac21e37a2a
  n60d788c38a --> n50a87bcbf6
  n6495b08fb4 --> n22e4a8e8ff
  n6495b08fb4 --> n50a87bcbf6
  n678ac9d473 --> n81797d6e71
  n6793031b47 --> n9dad8b7313
  n69e54add6a --> n81797d6e71
  n6c1dffcb0a --> n50a87bcbf6
  n6da73c81ea --> n3aec5e6c50
  n6da73c81ea --> n574112e035
  n6da73c81ea --> n941209c201
  n6da73c81ea --> na4e1f5b2a8
  n6da73c81ea --> nac21e37a2a
  n6da73c81ea --> nd41c7b3338
  n6dbe6a9578 --> n84e1213c58
  n6fd7dae001 --> n50a87bcbf6
  n721cae00c8 --> n574112e035
  n721cae00c8 --> nd41c7b3338
  n78a8b9aa9e --> n6dbe6a9578
  n78a8b9aa9e --> nb4a1e74cf7
  n78a8b9aa9e --> nba74d7f0f8
  n78a8b9aa9e --> nc51207575a
  n79b8e2b7e4 --> nac1e082ebf
  n7bd724cf63 --> n11a7fd3b87
  n7bd724cf63 --> n4c43215331
  n7bd724cf63 --> n678ac9d473
  n7bd724cf63 --> n97dd046d7f
  n7bd724cf63 --> nf84cac3046
  n7c3e14c330 --> n81797d6e71
  n7d83a20d1d --> n00241b2850
  n7d83a20d1d --> n7c3e14c330
  n7d83a20d1d --> nab5bbc4af8
  n7d83a20d1d --> nac21e37a2a
  n7d83a20d1d --> nbf9263d294
  n7d83a20d1d --> ndf873e457e
  n815823909d --> n11a7fd3b87
  n8339df6195 --> n211ac890ff
  n8339df6195 --> n574112e035
  n8339df6195 --> na4e1f5b2a8
  n8339df6195 --> nac21e37a2a
  n8339df6195 --> nd1d5bda3d6
  n84e1213c58 --> nac1e082ebf
  n85f9e2f6af --> n81797d6e71
  n86083e5721 --> n3aec5e6c50
  n86083e5721 --> n918723df5e
  n87eeda9d0a -.-> n5cc81c5e47
  n88199824c6 --> n11a7fd3b87
  n8840e2b858 --> n11a7fd3b87
  n8840e2b858 --> n5cc81c5e47
  n8840e2b858 --> nc96a78182d
  n89d52f0023 --> n3863aa4d2b
  n89d52f0023 --> n6495b08fb4
  n89d52f0023 --> nb93c2d043c
  n8a55774203 --> n0ca5a9c07f
  n8a55774203 --> n47db3f7310
  n8a55774203 --> n918723df5e
  n8cef5211ce --> n054886fd9b
  n8cef5211ce --> n06f5cd7180
  n8cef5211ce --> n69e54add6a
  n8e2b4347e1 --> n47db3f7310
  n8e2b4347e1 --> nf2ae7e0680
  n8fec4a08ed --> n50a87bcbf6
  n9005492f16 --> n89d52f0023
  n90d37ece3c --> nb93c2d043c
  n918723df5e --> n1dd42260be
  n918723df5e --> n50a87bcbf6
  n91caf2e441 --> n50a87bcbf6
  n91e2659598 --> n054886fd9b
  n91e2659598 --> n06f5cd7180
  n91e2659598 --> n28d1dcf52b
  n91e2659598 --> n8e2b4347e1
  n91e2659598 --> na6412ea892
  n91e2659598 --> nd0b1b13b33
  n9222a02d25 --> n3e89fea1ba
  n9222a02d25 --> n4303201550
  n9222a02d25 --> n5cc81c5e47
  n9222a02d25 --> n8a55774203
  n9222a02d25 --> nd5db088795
  n9222a02d25 --> nfb667eae25
  n926494aa19 --> n47db3f7310
  n92c3726b2a --> n84e1213c58
  n93eb1479e9 --> na6412ea892
  n941209c201 --> n20e1d4f1bd
  n941209c201 --> n574112e035
  n941209c201 --> n721cae00c8
  n941209c201 --> n91caf2e441
  n941209c201 --> na4e1f5b2a8
  n941209c201 --> nac21e37a2a
  n941209c201 --> nd1d5bda3d6
  n9434a0b72a --> n054886fd9b
  n9434a0b72a --> n5cc81c5e47
  n9434a0b72a --> n90d37ece3c
  n9434a0b72a --> nd41c7b3338
  n9581512a9e --> n11a7fd3b87
  n96cf03ffa9 --> n576b15c811
  n96cf03ffa9 --> n91caf2e441
  n96cf03ffa9 --> nac21e37a2a
  n9768753665 --> nab39be2bb6
  n97dd046d7f --> n81797d6e71
  n99b1f18848 --> n11a7fd3b87
  n9c5f83b7c9 --> n5dd3a6928b
  n9c5f83b7c9 --> n6495b08fb4
  n9c5f83b7c9 --> nb93c2d043c
  n9ca71c4ce2 --> n332e428c8f
  n9ca71c4ce2 --> n37f1acf921
  n9ca71c4ce2 --> n498dc74939
  n9ca71c4ce2 --> n81797d6e71
  n9ca71c4ce2 --> n8a55774203
  n9ca71c4ce2 --> nc654c3b31c
  n9eb1fa88dd --> n1dd42260be
  n9eb1fa88dd --> n50a87bcbf6
  n9eb1fa88dd --> n531233f1e6
  na20c316ef7 --> n1dd42260be
  na20c316ef7 --> n50a87bcbf6
  na228d598b6 --> n11a7fd3b87
  na4e1f5b2a8 --> n47db3f7310
  naa4f1bd8ff --> n3aec5e6c50
  naa4f1bd8ff --> n574112e035
  naa4f1bd8ff --> n5cc81c5e47
  naa4f1bd8ff --> n60d788c38a
  naa4f1bd8ff --> n91caf2e441
  naa4f1bd8ff --> n9dad8b7313
  naa4f1bd8ff --> na20c316ef7
  naa4f1bd8ff --> nda6b3ac5df
  naa4f1bd8ff --> ndf873e457e
  nab5bbc4af8 --> n50a87bcbf6
  nab5bbc4af8 --> nde71a3f98d
  nabe7085c3c --> n00241b2850
  nabe7085c3c --> n7c3e14c330
  nabe7085c3c --> n91caf2e441
  nabe7085c3c --> nd41c7b3338
  nb74aadb041 --> n35c6ea2d59
  nb74aadb041 --> n90d37ece3c
  nb74aadb041 --> na4e1f5b2a8
  nb74aadb041 --> nd997de33d8
  nb844e37ed3 --> n79b8e2b7e4
  nb844e37ed3 --> nac1e082ebf
  nbf9263d294 --> n81797d6e71
  nc0c8ddce55 --> n11a7fd3b87
  nc51207575a --> nb844e37ed3
  nc654c3b31c --> n332e428c8f
  nc654c3b31c --> n8d63ce830a
  nc96a78182d --> n3aec5e6c50
  nc96a78182d --> n5ca21e0784
  nc96a78182d --> nd41c7b3338
  nc96a78182d --> nd997de33d8
  nc96a78182d --> nf3ee2fe442
  nc990202d07 --> nac21e37a2a
  nca0045ec9e --> nac21e37a2a
  nca0045ec9e --> nea7899285e
  nca72b657ff --> n47db3f7310
  nca72b657ff --> nac21e37a2a
  ncfb946487b --> n11a7fd3b87
  nd0b1b13b33 --> n054886fd9b
  nd1d5bda3d6 --> n91caf2e441
  nd1d5bda3d6 --> ndf873e457e
  nd1e827a85e --> n26913ceebe
  nd1e827a85e --> n479954f0d2
  nd1e827a85e --> n50a87bcbf6
  nd263ebfd5e --> n5cc81c5e47
  nd41c7b3338 --> n47db3f7310
  nd41c7b3338 --> n5dd3a6928b
  nd41c7b3338 --> n9005492f16
  nd41c7b3338 --> n918723df5e
  nd41c7b3338 --> n91caf2e441
  nd41c7b3338 --> nac21e37a2a
  nd41c7b3338 --> nc51207575a
  nd41c7b3338 --> nea7899285e
  nd41c7b3338 --> neecb007ecd
  nd41c7b3338 --> nfa0fd60de8
  nd4a1b8d0ef -.-> nb44dcd1833
  nd4a1b8d0ef -.-> nb74aadb041
  nd5915cad45 --> n79b8e2b7e4
  nd5915cad45 --> nac1e082ebf
  nd922b8146c --> n89d52f0023
  nd922b8146c --> nd41c7b3338
  nda6b3ac5df --> n50a87bcbf6
  ndba31c55ba --> n91caf2e441
  ndba31c55ba --> n9dad8b7313
  ndba31c55ba --> nac21e37a2a
  ndba31c55ba --> nea93ccbc40
  ndcccba356c --> n6a0e117aa8
  ndd0a0247c8 --> n11a7fd3b87
  ndd0a0247c8 --> nf84cac3046
  nddc71c4bc6 --> nb93c2d043c
  nddc71c4bc6 --> nd41c7b3338
  nde71a3f98d --> n11a7fd3b87
  nde71a3f98d --> nab5bbc4af8
  nde71a3f98d --> nbf9263d294
  ndf873e457e --> n81797d6e71
  ne0599e1541 -.-> nd1e827a85e
  ne105c688d8 --> n11a7fd3b87
  ne105c688d8 --> nd41c7b3338
  ne57b2b2018 --> n0ca5a9c07f
  ne5be340822 -.-> n11a7fd3b87
  ne5be340822 -.-> nb44dcd1833
  ne5be340822 -.-> nb74aadb041
  ne5be340822 -.-> nd4a1b8d0ef
  ne5be340822 -.-> nfe20eab7ab
  ne6580eac0b -.-> n6f46fd19a8
  ne9f0365e4c --> n11a7fd3b87
  ne9f0365e4c --> n159be3f1e1
  nea7899285e --> n50a87bcbf6
  nf2ae7e0680 --> n211ac890ff
  nf2ae7e0680 --> n574112e035
  nf2ae7e0680 --> na4e1f5b2a8
  nf2ae7e0680 --> nac21e37a2a
  nf2ae7e0680 --> nd1d5bda3d6
  nf2ae7e0680 --> nd41c7b3338
  nf2ae7e0680 --> nea7899285e
  nf3ee2fe442 --> n3aec5e6c50
  nf3ee2fe442 --> n47db3f7310
  nf3ee2fe442 --> n8339df6195
  nf3ee2fe442 --> n918723df5e
  nf3ee2fe442 --> na4e1f5b2a8
  nf3ee2fe442 --> nda6b3ac5df
  nf84cac3046 --> n81797d6e71
  nfa0fd60de8 --> n50a87bcbf6
  nfa70e41c0f --> n11a7fd3b87
  nfa70e41c0f --> n47db3f7310
  nfa70e41c0f --> n91e2659598
  nfa70e41c0f --> na4e1f5b2a8
  nfb676358e5 --> n37f1acf921
  nfb676358e5 --> n6793031b47
  nfb676358e5 --> n91caf2e441
  nfb676358e5 --> nc01be743ca
  nfb676358e5 --> ndf873e457e
  nfb676358e5 --> nf6fc8bbb4a
  nfbd041d4ef --> n00241b2850
  nfc5c76ccd9 --> n11a7fd3b87
  nfc5c76ccd9 --> n2ea2a910cf
  nfc5c76ccd9 --> n7c3e14c330
  nfe20eab7ab -.-> n5dd3a6928b
  classDef alto fill:#5b1f2a,stroke:#ff7a7a,color:#fff
  classDef medio fill:#4a3717,stroke:#ffca67,color:#fff
  classDef bajo fill:#123b3a,stroke:#39e6aa,color:#fff
  class n4303201550 bajo
  class nd5db088795 bajo
  class n9222a02d25 bajo
  class n3e89fea1ba bajo
  class nfb667eae25 bajo
  class nfb676358e5 bajo
  class nf6fc8bbb4a bajo
  class nc01be743ca bajo
  class n8d63ce830a bajo
  class n332e428c8f bajo
  class n91f6dd4e52 bajo
  class nc654c3b31c bajo
  class n9ca71c4ce2 medio
  class n9dad8b7313 bajo
  class n6793031b47 medio
  class n720b5e0530 bajo
  class n1dd42260be bajo
  class n81797d6e71 bajo
  class n50a87bcbf6 bajo
  class n37f1acf921 bajo
  class ndf873e457e bajo
  class n91caf2e441 bajo
  class n9eb1fa88dd bajo
  class n6fd7dae001 bajo
  class n918723df5e bajo
  class n2c767e452a bajo
  class nfa0fd60de8 bajo
  class nea7899285e bajo
  class nda6b3ac5df bajo
  class na20c316ef7 bajo
  class n3aec5e6c50 medio
  class n40030491a7 bajo
  class n1adac0dfef bajo
  class n26913ceebe bajo
  class n479954f0d2 bajo
  class nd1e827a85e bajo
  class nf84cac3046 bajo
  class n85f9e2f6af bajo
  class n4c43215331 bajo
  class n97dd046d7f bajo
  class n678ac9d473 bajo
  class n6c1dffcb0a bajo
  class n6495b08fb4 bajo
  class n22e4a8e8ff bajo
  class n69e54add6a bajo
  class n46184faee8 bajo
  class n8fec4a08ed bajo
  class n60d788c38a bajo
  class n498dc74939 bajo
  class nbf9263d294 bajo
  class n7c3e14c330 bajo
  class nab5bbc4af8 bajo
  class n2ea2a910cf bajo
  class nac1e082ebf bajo
  class n79b8e2b7e4 bajo
  class nb844e37ed3 bajo
  class nc51207575a bajo
  class n6f46fd19a8 bajo
  class n84e1213c58 bajo
  class nd5915cad45 medio
  class n35c6ea2d59 medio
  class n9db805ee4f bajo
  class n5cc81c5e47 medio
  class n0ca5a9c07f medio
  class nea93ccbc40 bajo
  class neecb007ecd medio
  class n211ac890ff bajo
  class nd997de33d8 bajo
  class n5d15be8f9a medio
  class n374a8c751b medio
  class ne6580eac0b bajo
  class n77c69d98ee medio
  class n719a3019d3 bajo
  class n9daf052028 bajo
  class na6412ea892 bajo
  class nb4a1e74cf7 bajo
  class nba74d7f0f8 bajo
  class n6dbe6a9578 bajo
  class n78a8b9aa9e medio
  class n0e1fa47dee bajo
  class n28d1dcf52b bajo
  class n5c0fbdc43d bajo
  class n3e673f6e4a bajo
  class n92c3726b2a bajo
  class n3863aa4d2b bajo
  class nfa2e0f121a bajo
  class nfe20eab7ab bajo
  class nb44dcd1833 bajo
  class nac21e37a2a bajo
  class n87eeda9d0a medio
  class n14396af8ec bajo
  class n91340c1852 medio
  class ne0599e1541 bajo
  class nbd38f1954c bajo
  class n9c5f83b7c9 bajo
  class n5dd3a6928b bajo
  class n9005492f16 bajo
  class n89d52f0023 bajo
  class nb93c2d043c bajo
  class n90d37ece3c bajo
  class naa4f1bd8ff bajo
  class n8a55774203 medio
  class na58c542b94 bajo
  class ndba31c55ba bajo
  class n576b15c811 bajo
  class n96cf03ffa9 bajo
  class n1609e58b46 bajo
  class ne57b2b2018 medio
  class n226e0c2b69 bajo
  class nb74aadb041 medio
  class na4e1f5b2a8 bajo
  class n574112e035 bajo
  class nc8bc2a0782 bajo
  class n054886fd9b bajo
  class nd0b1b13b33 bajo
  class n93eb1479e9 bajo
  class n06f5cd7180 medio
  class n91e2659598 bajo
  class n9434a0b72a bajo
  class nddc71c4bc6 bajo
  class nd922b8146c bajo
  class n6a0e117aa8 bajo
  class ndcccba356c bajo
  class n5c439ba5fa bajo
  class n2c90211809 bajo
  class n8cef5211ce bajo
  class n159be3f1e1 medio
  class n47db3f7310 medio
  class n926494aa19 bajo
  class n941209c201 medio
  class nd1d5bda3d6 bajo
  class n20e1d4f1bd bajo
  class n721cae00c8 bajo
  class n8e2b4347e1 bajo
  class nf2ae7e0680 medio
  class n0a7c80fcad medio
  class n5fd3ee00e7 medio
  class nd41c7b3338 medio
  class n4885a4566a medio
  class nca72b657ff medio
  class n264336e253 bajo
  class nd263ebfd5e bajo
  class n86083e5721 bajo
  class n3484b80fe4 bajo
  class nca0045ec9e bajo
  class n05fa14e75a medio
  class n6da73c81ea medio
  class n3d50adb5e0 medio
  class n5ca21e0784 bajo
  class n8339df6195 medio
  class nf3ee2fe442 medio
  class nc96a78182d medio
  class ncfcaf08821 bajo
  class n00241b2850 bajo
  class nfbd041d4ef bajo
  class n2b2d1dce7b bajo
  class n27c57d90b7 bajo
  class nc990202d07 bajo
  class nabe7085c3c bajo
  class n7d83a20d1d bajo
  class n11a7fd3b87 bajo
  class n9768753665 bajo
  class nab39be2bb6 bajo
  class nef186c8e4b bajo
  class nda183cb233 bajo
  class n815823909d bajo
  class n1eb416b959 bajo
  class n0cf4ca5f46 bajo
  class n33765df17c bajo
  class ne105c688d8 medio
  class n36fb8ff079 bajo
  class n9581512a9e bajo
  class n5b56b0b4a6 bajo
  class n99b1f18848 bajo
  class n0ae797aea0 bajo
  class ncfb946487b bajo
  class n88199824c6 bajo
  class n5ac028934f bajo
  class nfa70e41c0f bajo
  class n2cf25eaa6f bajo
  class ne9f0365e4c bajo
  class n8840e2b858 bajo
  class na228d598b6 bajo
  class nc0c8ddce55 bajo
  class n024799fea3 bajo
  class nde71a3f98d bajo
  class nfc5c76ccd9 bajo
  class n0bec3f253d bajo
  class ndd0a0247c8 medio
  class n531233f1e6 bajo
  class n7bd724cf63 medio
  class ne5be340822 bajo
  class nd4a1b8d0ef bajo
```

Fuentes: [Mermaid](mermaid/backend_python.mmd) · [PlantUML](plantuml/backend_python.puml). Las flechas continuas son llamadas síncronas; las discontinuas representan asincronía, eventos o colas. El color del nodo indica riesgo estático.

## Inventario

| Función | Archivo | CC | Propietario | Riesgo/estado | Entra desde | Sale hacia | Estado compartido |
|---|---|---:|---|---|---|---|---|
| `_load_application` | [`desktop_app/app.py`](../../desktop_app/app.py#L10) | 3 | Python / misión e historial | Bajo; interno; síncrona | `main` | — | — |
| `parse_args` | [`desktop_app/app.py`](../../desktop_app/app.py#L31) | 1 | Python / misión e historial | Bajo; interno; síncrona | `main` | — | — |
| `main` | [`desktop_app/app.py`](../../desktop_app/app.py#L39) | 4 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_load_application`, `close`, `create_app`, `parse_args`, `run`, `shutdown_host`, `start` | parada/cierre |
| `main.shutdown_host` | [`desktop_app/app.py`](../../desktop_app/app.py#L50) | 3 | Python / misión e historial | Bajo; interno; síncrona | `main` | `close` | parada/cierre |
| `create_app` | [`desktop_app/robot_app/__init__.py`](../../desktop_app/robot_app/__init__.py#L6) | 1 | Python / misión e historial | Bajo; interno; síncrona | `main`, `run` | — | — |
| `create_app` | [`desktop_app/robot_app/app_factory.py`](../../desktop_app/robot_app/app_factory.py#L17) | 2 | Python / misión e historial | Bajo; interno; síncrona | `main`, `run` | `create_csp_nonce`, `from_environment`, `get_setting`, `initialize`, `secure_headers`, `set_setting`, `update` | — |
| `create_app.create_csp_nonce` | [`desktop_app/robot_app/app_factory.py`](../../desktop_app/robot_app/app_factory.py#L41) | 1 | Python / misión e historial | Bajo; interno; síncrona | `create_app` | — | — |
| `create_app.secure_headers` | [`desktop_app/robot_app/app_factory.py`](../../desktop_app/robot_app/app_factory.py#L45) | 1 | Python / misión e historial | Bajo; interno; síncrona | `create_app` | — | — |
| `_quantize` | [`desktop_app/robot_app/compaction.py`](../../desktop_app/robot_app/compaction.py#L15) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_telemetry_key`, `telemetry_fingerprint` | — | — |
| `_normalized_payload` | [`desktop_app/robot_app/compaction.py`](../../desktop_app/robot_app/compaction.py#L22) | 8 | Python / misión e historial | Bajo; interno; síncrona | `_telemetry_key`, `compact_database` | `clean` | — |
| `_normalized_payload.clean` | [`desktop_app/robot_app/compaction.py`](../../desktop_app/robot_app/compaction.py#L33) | 7 | Python / misión e historial | Bajo; interno; síncrona | `_normalized_payload` | — | — |
| `_telemetry_key` | [`desktop_app/robot_app/compaction.py`](../../desktop_app/robot_app/compaction.py#L45) | 2 | Python / misión e historial | Bajo; interno; síncrona | `compact_database` | `_normalized_payload`, `_quantize` | — |
| `compact_database` | [`desktop_app/robot_app/compaction.py`](../../desktop_app/robot_app/compaction.py#L60) | 32 | Python / misión e historial | Medio; sin llamada interna detectada; síncrona | — | `_normalized_payload`, `_telemetry_key`, `close`, `connect`, `initialize`, `optimize_storage`, `update` | telemetría, parada/cierre |
| `normalize_robot_host` | [`desktop_app/robot_app/config.py`](../../desktop_app/robot_app/config.py#L17) | 6 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `from_environment`, `set_robot_host` | — | — |
| `AppConfig.from_environment` | [`desktop_app/robot_app/config.py`](../../desktop_app/robot_app/config.py#L41) | 1 | Python / misión e historial | Medio; interno; asíncrona | `create_app` | `normalize_robot_host` | — |
| `Database.__init__` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L15) | 1 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Database._session_timestamp` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L20) | 2 | Python / misión e historial | Bajo; interno; síncrona | `close_orphan_sessions`, `create_session`, `stop_session` | — | — |
| `Database.connect` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L31) | 1 | Python / misión e historial | Bajo; interno; síncrona | `command_rows`, `compact_database`, `connect_db`, `event_rows`, `get_calibration_surface`, `get_setting`, `initialize`, `list_calibration_surfaces`, `main`, `optimize_storage`, `session_row`, `session_rows`, `telemetry_rows`, `touch_recording`, `touch_recordings`, `transaction` | — | — |
| `Database.transaction` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L41) | 3 | Python / misión e historial | Bajo; interno; síncrona | `acknowledge_touch_recording`, `close_orphan_sessions`, `create_session`, `delete_calibration_surface`, `delete_touch_recording`, `fail_nonterminal_commands`, `insert_command`, `insert_event`, `insert_telemetry`, `invalidate_open_touch_recordings`, `purge_sessions`, `save_calibration_surface`, `save_touch_recording`, `set_setting`, `stop_session`, `update_command`, `update_session_disconnect_reason`, `update_session_identity` | `close`, `connect` | — |
| `Database.initialize` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L54) | 20 | Python / misión e historial | Bajo; interno; síncrona | `compact_database`, `create_app` | `calibration_surfaces`, `close`, `connect`, `touch_recordings` | telemetría |
| `Database.get_setting` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L147) | 3 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `_activate_mission`, `_set_return_state`, `_sync_surface_torque_from_telemetry`, `create_app`, `start_return_home`, `stop_mission` | `connect` | parada/cierre |
| `Database.set_setting` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L152) | 3 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `_activate_mission`, `_advance_mission`, `_persist_mission`, `_set_return_state`, `apply_calibration_surface`, `create_app`, `send_command`, `set_robot_host`, `set_vectorial_routes_enabled`, `start_return_home`, `stop_mission` | `transaction` | — |
| `Database.create_session` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L161) | 5 | Python / misión e historial | Bajo; interno; síncrona | `start_session` | `_session_timestamp`, `sessions`, `transaction` | — |
| `Database.update_session_identity` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L171) | 5 | Python / misión e historial | Bajo; interno; síncrona | `_on_robot_message` | `transaction` | — |
| `Database.stop_session` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L178) | 7 | Python / misión e historial | Bajo; entrada/framework; síncrona | `_finish_close_preparation`, `_on_connection_state`, `_on_disconnect_grace_expired`, `_on_robot_message`, `close`, `send_command` | `_session_timestamp`, `transaction` | — |
| `Database.update_session_disconnect_reason` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L188) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_on_connection_state` | `transaction` | — |
| `Database.insert_command` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L196) | 6 | Python / misión e historial | Bajo; interno; síncrona | `send_command` | `transaction` | — |
| `Database.update_command` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L203) | 6 | Python / misión e historial | Bajo; interno; síncrona | `_block_mission`, `_on_command_sent`, `_on_robot_message`, `_reconcile_short_memory_hello`, `send_command`, `stop_mission` | `transaction` | — |
| `Database.fail_nonterminal_commands` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L215) | 2 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `_finish_close_preparation` | `transaction` | — |
| `Database.close_orphan_sessions` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L225) | 6 | Python / misión e historial | Bajo; interno; síncrona | `__init__` | `_session_timestamp`, `transaction` | — |
| `Database.insert_event` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L235) | 17 | Python / misión e historial | Medio; interno; síncrona | `__init__`, `_finish_close_preparation`, `_on_connection_state`, `_on_disconnect_grace_expired`, `_on_robot_message`, `_on_step_settled`, `_reconcile_short_memory_hello`, `prepare_close` | `events`, `transaction` | parada/cierre |
| `Database._quantize` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L280) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_telemetry_key`, `telemetry_fingerprint` | — | — |
| `Database.telemetry_priority_signature` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L287) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_on_robot_message`, `telemetry_fingerprint` | — | — |
| `Database.telemetry_fingerprint` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L308) | 3 | Python / misión e historial | Bajo; interno; síncrona | `insert_telemetry` | `_quantize`, `telemetry_priority_signature` | — |
| `Database._compact_payload` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L325) | 5 | Python / misión e historial | Bajo; interno; síncrona | `insert_telemetry` | — | telemetría |
| `Database.insert_telemetry` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L342) | 30 | Python / misión e historial | Bajo; interno; síncrona | `_run` | `_compact_payload`, `telemetry_fingerprint`, `transaction` | telemetría |
| `Database.telemetry_rows` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L373) | 2 | Python / misión e historial | Bajo; interno; síncrona | `export_session_json`, `export_telemetry` | `connect` | telemetría, parada/cierre |
| `Database.session_rows` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L384) | 1 | Python / misión e historial | Bajo; interno; síncrona | `sessions` | `connect` | telemetría, parada/cierre |
| `Database.session_row` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L396) | 2 | Python / misión e historial | Bajo; interno; síncrona | `export_session_json` | `connect` | parada/cierre |
| `Database.command_rows` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L400) | 2 | Python / misión e historial | Bajo; interno; síncrona | `export_session_json` | `connect` | parada/cierre |
| `Database.event_rows` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L407) | 2 | Python / misión e historial | Bajo; interno; síncrona | `export_session_json` | `connect` | parada/cierre |
| `Database.purge_sessions` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L416) | 9 | Python / misión e historial | Bajo; interno; síncrona | `cleanup_sessions` | `transaction` | telemetría |
| `Database.save_touch_recording` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L437) | 18 | Python / misión e historial | Bajo; interno; síncrona | `finish`, `start` | `touch_recordings`, `transaction` | — |
| `Database.touch_recordings` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L464) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | `initialize`, `save_touch_recording` | `connect` | parada/cierre |
| `Database.touch_recording` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L468) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | `acknowledge_open_area`, `return_touch_recording` | `connect` | parada/cierre |
| `Database.delete_touch_recording` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L472) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `transaction` | — |
| `Database.acknowledge_touch_recording` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L477) | 2 | Python / misión e historial | Bajo; interno; síncrona | `acknowledge_open_area` | `transaction` | — |
| `Database.invalidate_open_touch_recordings` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L482) | 2 | Python / misión e historial | Bajo; interno; síncrona | `__init__` | `transaction` | — |
| `Database.optimize_storage` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L491) | 4 | Python / misión e historial | Bajo; interno; síncrona | `compact_database` | `close`, `connect` | — |
| `Database.list_calibration_surfaces` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L512) | 1 | Python / misión e historial | Bajo; interno; síncrona | `_sync_surface_torque_from_telemetry`, `calibration_surfaces` | `connect` | parada/cierre |
| `Database.get_calibration_surface` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L516) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_sync_surface_torque_from_telemetry`, `apply_calibration_surface`, `calibration_surface_item`, `save_calibration_surface` | `connect` | parada/cierre |
| `Database.save_calibration_surface` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L520) | 15 | Python / misión e historial | Bajo; interno; síncrona | `_sync_surface_torque_from_telemetry`, `calibration_surfaces` | `calibration_surfaces`, `transaction` | — |
| `Database.delete_calibration_surface` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L556) | 2 | Python / misión e historial | Bajo; interno; síncrona | `calibration_surface_item` | `transaction` | — |
| `_finite_number` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L54) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_heading_degrees`, `compile`, `from_message`, `split_segment_mm`, `validate_command_payload` | — | — |
| `_heading_degrees` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L63) | 1 | Python / misión e historial | Bajo; interno; síncrona | `from_message`, `validate_command_payload` | `_finite_number` | — |
| `validate_command_payload` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L67) | 29 | Python / misión e historial | Bajo; interno; síncrona | `create` | `_finite_number`, `_heading_degrees` | — |
| `RobotCommand.create` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L158) | 3 | Python / misión e historial | Bajo; interno; síncrona | `compile`, `send_command` | `validate_command_payload` | — |
| `RobotCommand.protocol_envelope` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L172) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_drain_one` | — | — |
| `split_segment_mm` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L178) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_append_segment`, `compile_orthogonal_points` | `_finite_number` | — |
| `TelemetrySnapshot.from_message` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L307) | 46 | Python / misión e historial | Medio; interno; síncrona | `_on_robot_message` | `_finite_number`, `_heading_degrees` | telemetría |
| `TelemetrySnapshot.public_dict` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L493) | 1 | Python / misión e historial | Medio; interno; síncrona | `_on_robot_message`, `status` | — | — |
| `RobotGateway.__init__` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L38) | 1 | Python / misión e historial | Bajo; sin llamada interna detectada; cola/evento | — | — | — |
| `RobotGateway.start` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L70) | 3 | Python / misión e historial | Medio; interno; asíncrona | `__init__`, `_on_robot_message`, `_schedule_disconnect_grace`, `close_application`, `connect`, `init`, `main`, `manual_start`, `reconnect` | `clear`, `snapshot` | — |
| `RobotGateway.stop` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L80) | 2 | Python / misión e historial | Medio; interno; síncrona | `close`, `disconnect` | `_set_state` | — |
| `RobotGateway.request_reconnect` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L87) | 2 | Python / misión e historial | Bajo; interno; síncrona | `reconnect`, `set_robot_host` | — | — |
| `RobotGateway.enqueue` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L91) | 6 | Python / misión e historial | Medio; interno; cola/evento | `send_command` | — | parada/cierre |
| `RobotGateway.cancel` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L108) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_activate_mission`, `_archive_mission_for_close`, `_block_mission`, `_cancel_disconnect_grace_timer`, `_on_robot_message`, `stop_mission` | `add` | — |
| `RobotGateway.snapshot` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L113) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_on_robot_message`, `_require_ready_robot`, `prepare_close`, `reconnect`, `start`, `status` | — | — |
| `RobotGateway._set_state` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L129) | 3 | Python / misión e historial | Medio; interno; asíncrona | `_run`, `stop` | — | — |
| `RobotGateway._run` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L136) | 15 | Python / misión e historial | Medio; sin llamada interna detectada; asíncrona | — | `_drain_one`, `_receive`, `_set_state`, `clear`, `close`, `send` | telemetría, sesión |
| `RobotGateway._drain_one` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L217) | 13 | Python / misión e historial | Bajo; interno; cola/evento | `_run` | `protocol_envelope`, `send` | — |
| `RobotGateway._receive` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L264) | 11 | Python / misión e historial | Medio; interno; síncrona | `_run` | — | sesión |
| `_finite_number` | [`desktop_app/robot_app/route_planning.py`](../../desktop_app/robot_app/route_planning.py#L24) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_heading_degrees`, `compile`, `from_message`, `split_segment_mm`, `validate_command_payload` | — | — |
| `normalize_signed_degrees` | [`desktop_app/robot_app/route_planning.py`](../../desktop_app/robot_app/route_planning.py#L33) | 2 | Python / misión e historial | Bajo; interno; síncrona | `compile` | — | — |
| `RouteExecutionStrategy.compile` | [`desktop_app/robot_app/route_planning.py`](../../desktop_app/robot_app/route_planning.py#L51) | 1 | Python / misión e historial | Bajo; interno; síncrona | `_build_segments`, `embedded_html_functions`, `extract_routes`, `start_mission` | — | — |
| `RouteExecutionStrategy._validated_origin` | [`desktop_app/robot_app/route_planning.py`](../../desktop_app/robot_app/route_planning.py#L55) | 1 | Python / misión e historial | Bajo; interno; síncrona | `compile` | — | — |
| `RouteExecutionStrategy._finish` | [`desktop_app/robot_app/route_planning.py`](../../desktop_app/robot_app/route_planning.py#L59) | 3 | Python / misión e historial | Bajo; interno; síncrona | `compile` | — | — |
| `RouteExecutionStrategy._append_segment` | [`desktop_app/robot_app/route_planning.py`](../../desktop_app/robot_app/route_planning.py#L68) | 2 | Python / misión e historial | Bajo; interno; síncrona | `compile` | `split_segment_mm` | — |
| `RectangularRouteStrategy.compile` | [`desktop_app/robot_app/route_planning.py`](../../desktop_app/robot_app/route_planning.py#L94) | 9 | Python / misión e historial | Medio; interno; síncrona | `_build_segments`, `embedded_html_functions`, `extract_routes`, `start_mission` | `_append_segment`, `_finish`, `_validated_origin`, `create` | — |
| `_AngularRouteStrategy.compile` | [`desktop_app/robot_app/route_planning.py`](../../desktop_app/robot_app/route_planning.py#L141) | 10 | Python / misión e historial | Bajo; interno; síncrona | `_build_segments`, `embedded_html_functions`, `extract_routes`, `start_mission` | `_append_segment`, `_finish`, `_finite_number`, `_validated_origin`, `create`, `normalize_signed_degrees` | — |
| `strategy_for_request` | [`desktop_app/robot_app/route_planning.py`](../../desktop_app/robot_app/route_planning.py#L217) | 3 | Python / misión e historial | Bajo; interno; síncrona | `start_mission` | — | — |
| `simplify_rdp` | [`desktop_app/robot_app/route_planning.py`](../../desktop_app/robot_app/route_planning.py#L228) | 8 | Python / misión e historial | Bajo; interno; síncrona | `compile_touch_path` | `add`, `distance` | — |
| `simplify_rdp.distance` | [`desktop_app/robot_app/route_planning.py`](../../desktop_app/robot_app/route_planning.py#L234) | 2 | Python / misión e historial | Bajo; interno; síncrona | `simplify_rdp` | — | — |
| `compile_orthogonal_points` | [`desktop_app/robot_app/route_planning.py`](../../desktop_app/robot_app/route_planning.py#L259) | 10 | Python / misión e historial | Bajo; interno; síncrona | `compile_touch_path` | `split_segment_mm` | — |
| `compile_touch_path` | [`desktop_app/robot_app/route_planning.py`](../../desktop_app/robot_app/route_planning.py#L291) | 7 | Python / misión e historial | Bajo; interno; síncrona | `finish` | `compile_orthogonal_points`, `simplify_rdp` | — |
| `EventHub.__init__` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L25) | 1 | Python / misión e historial | Bajo; sin llamada interna detectada; cola/evento | — | — | — |
| `EventHub.subscribe` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L30) | 1 | Python / misión e historial | Bajo; interno; cola/evento | `events` | `add` | — |
| `EventHub.unsubscribe` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L36) | 1 | Python / misión e historial | Bajo; interno; cola/evento | `events`, `stream` | — | — |
| `EventHub.publish` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L40) | 4 | Python / misión e historial | Bajo; interno; cola/evento | `_activate_mission`, `_advance_mission`, `_archive_mission_for_close`, `_block_mission`, `_on_command_sent`, `_on_connection_state`, `_on_robot_message`, `_on_step_settled`, `_reconcile_firmware_mission`, `_sync_surface_torque_from_telemetry`, `delete_calibration_surface`, `save_calibration_surface`, `send_command`, `set_robot_host`, `set_vectorial_routes_enabled`, `start_session`, `stop_mission`, `stop_session` | — | — |
| `TelemetryRecorder.__init__` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L56) | 1 | Python / misión e historial | Medio; sin llamada interna detectada; cola/evento | — | `start` | telemetría |
| `TelemetryRecorder.submit` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L64) | 3 | Python / misión e historial | Bajo; interno; cola/evento | `_on_robot_message` | — | — |
| `TelemetryRecorder.stop` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L74) | 1 | Python / misión e historial | Medio; interno; síncrona | `close`, `disconnect` | — | — |
| `TelemetryRecorder._run` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L78) | 4 | Python / misión e historial | Bajo; sin llamada interna detectada; cola/evento | — | `insert_telemetry` | — |
| `TouchRouteRecorder.__init__` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L95) | 1 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `TouchRouteRecorder.start` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L100) | 2 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `_on_robot_message`, `_schedule_disconnect_grace`, `close_application`, `connect`, `init`, `main`, `manual_start`, `reconnect` | `add`, `public`, `save_touch_recording` | — |
| `TouchRouteRecorder.add` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L113) | 5 | Python / misión e historial | Bajo; interno; síncrona | `_on_robot_message`, `_process`, `_sendManual`, `_syncCinemaButton`, `addStep`, `applyBackendTelemetry`, `applySelected`, `applyTelemetry`, `cancel`, `cancelOckhamReturn`, `captureFromTelemetry`, `change`, `clearRobotMemory`, `closeApplication`, `confirmOckhamReturn`, `connectBackend`, `consumeBackend`, `deleteRecording`, `deleteSelected`, `dependency_cycles`, `end`, `failRoute`, `handleCalibrationCompleted`, `hlStep`, `init`, `initResize`, `loadHistory`, `loadSurfaces`, `lockTabs`, `loop`, `mainW`, `mermaid_for_folder`, `ockhamReturn`, `onDown`, `onMission`, `onRobotEvent`, `pL`, `pR`, `removeLastStep`, `returnRecording`, `saveCurrent`, `saveCurrentLiveSurface`, `send`, `sendMove`, `send_command`, `simplify_rdp`, `start`, `startC`, `stopAndReset`, `subscribe`, `update`, `visit` | `finish` | — |
| `TouchRouteRecorder.invalidate` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L135) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_on_connection_state`, `_on_robot_message`, `send_command` | `finish` | — |
| `TouchRouteRecorder.finish` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L139) | 11 | Python / misión e historial | Bajo; interno; síncrona | `add`, `invalidate`, `manual_stop` | `compile_touch_path`, `public`, `save_touch_recording`, `update` | — |
| `TouchRouteRecorder.public` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L168) | 2 | Python / misión e historial | Bajo; interno; síncrona | `finish`, `manual_drive`, `start`, `summary` | — | — |
| `TouchRouteRecorder.summary` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L175) | 1 | Python / misión e historial | Bajo; interno; síncrona | `_on_robot_message`, `manual_start`, `status` | `public` | — |
| `RobotService.__init__` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L195) | 17 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `_persist_mission`, `close_orphan_sessions`, `fail_nonterminal_commands`, `get_setting`, `insert_event`, `invalidate_open_touch_recordings`, `normalize_robot_host`, `set_setting`, `start` | misión activa |
| `RobotService.close` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L333) | 4 | Python / misión e historial | Medio; interno; síncrona | `_run`, `closeApplication`, `compact_database`, `escribirArchivo`, `initialize`, `leerArchivo`, `main`, `optimize_storage`, `run`, `shutdown_host`, `stop`, `transaction` | `stop`, `stop_mission`, `stop_session` | — |
| `RobotService.get_robot_host` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L348) | 1 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `RobotService.set_robot_host` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L352) | 2 | Python / misión e historial | Bajo; interno; síncrona | `robot_config` | `normalize_robot_host`, `publish`, `request_reconnect`, `set_setting` | — |
| `RobotService.robot_config` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L363) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | `set_vectorial_routes_enabled` | — | — |
| `RobotService.set_vectorial_routes_enabled` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L370) | 3 | Python / misión e historial | Bajo; interno; síncrona | `robot_config` | `publish`, `robot_config`, `set_setting` | — |
| `RobotService.connect` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L381) | 1 | Python / misión e historial | Bajo; interno; síncrona | `command_rows`, `compact_database`, `connect_db`, `event_rows`, `get_calibration_surface`, `get_setting`, `initialize`, `list_calibration_surfaces`, `main`, `optimize_storage`, `session_row`, `session_rows`, `telemetry_rows`, `touch_recording`, `touch_recordings`, `transaction` | `start` | — |
| `RobotService.disconnect` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L384) | 1 | Python / misión e historial | Medio; sin llamada interna detectada; síncrona | — | `stop` | — |
| `RobotService.reconnect` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L387) | 2 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `request_reconnect`, `snapshot`, `start` | — |
| `RobotService.status` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L393) | 2 | Python / misión e historial | Medio; entrada/framework; síncrona | `connection`, `events`, `stream` | `mission_status`, `public_dict`, `snapshot`, `summary` | telemetría, parada/cierre |
| `RobotService.mission_status` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L418) | 8 | Python / misión e historial | Bajo; interno; síncrona | `_activate_mission`, `_advance_mission`, `_archive_mission_for_close`, `_block_mission`, `_finish_close_preparation`, `_on_robot_message`, `_on_step_settled`, `_reconcile_firmware_mission`, `missions`, `status`, `stop_mission` | `stop_mission` | — |
| `RobotService._persist_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L452) | 2 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `_activate_mission`, `_advance_mission`, `_archive_mission_for_close`, `_block_mission`, `_on_robot_message`, `_on_step_settled`, `_queue_current_mission_step`, `_queue_final_alignment`, `_reconcile_firmware_mission`, `stop_mission` | `set_setting` | misión activa |
| `RobotService._mission_wire_points` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L469) | 2 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `RobotService._require_ready_robot` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L481) | 7 | Python / misión e historial | Bajo; interno; síncrona | `_require_vectorial_ready`, `manual_start`, `return_touch_recording`, `start_mission`, `start_return_home` | `snapshot` | estado, telemetría |
| `RobotService._require_vectorial_ready` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L499) | 8 | Python / misión e historial | Bajo; interno; síncrona | `_queue_current_mission_step`, `start_mission`, `start_return_home` | `_require_ready_robot` | telemetría |
| `RobotService._build_segments` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L519) | 1 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `compile` | — |
| `RobotService._activate_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L524) | 5 | Python / misión e historial | Medio; interno; síncrona | `return_touch_recording`, `start_mission`, `start_return_home` | `_persist_mission`, `_queue_current_mission_step`, `cancel`, `get_setting`, `mission_status`, `publish`, `set_setting` | — |
| `RobotService.start_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L570) | 4 | Python / misión e historial | Bajo; interno; síncrona | `missions` | `_activate_mission`, `_expire_stalled_mission`, `_require_ready_robot`, `_require_vectorial_ready`, `compile`, `strategy_for_request` | telemetría |
| `RobotService.manual_start` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L590) | 6 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_require_ready_robot`, `send_command`, `start`, `summary` | — |
| `RobotService.manual_drive` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L611) | 5 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `public`, `send_command` | — |
| `RobotService.manual_stop` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L632) | 3 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `finish`, `send_command` | — |
| `RobotService._touch_row` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L643) | 4 | Python / misión e historial | Bajo; interno; síncrona | `touch_recording`, `touch_recordings` | — | — |
| `RobotService.touch_recordings` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L656) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | `initialize`, `save_touch_recording` | `_touch_row` | — |
| `RobotService.touch_recording` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L659) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | `acknowledge_open_area`, `return_touch_recording` | `_touch_row` | — |
| `RobotService.acknowledge_open_area` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L665) | 3 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `acknowledge_touch_recording`, `touch_recording` | — |
| `RobotService.return_touch_recording` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L673) | 4 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_activate_mission`, `_require_ready_robot`, `touch_recording` | telemetría |
| `RobotService.start_return_home` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L695) | 21 | Python / misión e historial | Medio; interno; síncrona | `return_home` | `_activate_mission`, `_require_ready_robot`, `_require_vectorial_ready`, `get_setting`, `set_setting` | telemetría |
| `RobotService.stop_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L805) | 8 | Python / misión e historial | Medio; interno; síncrona | `_expire_stalled_mission`, `_finish_close_preparation`, `_on_robot_message`, `clear_robot_mission_memory`, `close`, `mission_status`, `missions`, `send_command`, `stop_session` | `_persist_mission`, `_set_return_state`, `cancel`, `get_setting`, `mission_status`, `publish`, `send_command`, `set_setting`, `update_command` | — |
| `RobotService.clear_robot_mission_memory` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L858) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `stop_mission` | — |
| `RobotService._advance_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L864) | 10 | Python / misión e historial | Medio; interno; síncrona | `_on_robot_message`, `_on_step_settled`, `_reconcile_short_memory_hello` | `_persist_mission`, `_queue_current_mission_step`, `_queue_final_alignment`, `_set_return_state`, `mission_status`, `publish`, `set_setting` | — |
| `RobotService._set_return_state` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L908) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_advance_mission`, `_archive_mission_for_close`, `_block_mission`, `stop_mission` | `get_setting`, `set_setting` | — |
| `RobotService._queue_current_mission_step` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L915) | 8 | Python / misión e historial | Bajo; interno; síncrona | `_activate_mission`, `_advance_mission`, `_reconcile_short_memory_hello` | `_block_mission`, `_persist_mission`, `_require_vectorial_ready`, `send_command` | telemetría |
| `RobotService._queue_final_alignment` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L953) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_advance_mission`, `_reconcile_short_memory_hello` | `_persist_mission`, `send_command` | — |
| `RobotService._expire_stalled_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L967) | 6 | Python / misión e historial | Bajo; interno; síncrona | `_on_robot_message`, `start_mission` | `_block_mission`, `stop_mission` | — |
| `RobotService._block_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L988) | 6 | Python / misión e historial | Medio; interno; síncrona | `_expire_stalled_mission`, `_on_robot_message`, `_queue_current_mission_step`, `_reconcile_mission_snapshot`, `_reconcile_short_memory_hello` | `_persist_mission`, `_set_return_state`, `cancel`, `mission_status`, `publish`, `send_command`, `update_command` | — |
| `RobotService._reconcile_mission_snapshot` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1016) | 8 | Python / misión e historial | Medio; sin llamada interna detectada; síncrona | — | `_block_mission`, `_reconcile_firmware_mission` | — |
| `RobotService._reconcile_firmware_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1042) | 9 | Python / misión e historial | Medio; interno; síncrona | `_reconcile_mission_snapshot` | `_persist_mission`, `mission_status`, `publish` | — |
| `RobotService.send_command` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1076) | 17 | Python / misión e historial | Medio; interno; síncrona | `_block_mission`, `_on_robot_message`, `_on_step_settled`, `_queue_current_mission_step`, `_queue_final_alignment`, `_reconcile_short_memory_hello`, `apply_calibration_surface`, `command`, `estop`, `manual_drive`, `manual_start`, `manual_stop`, `prepare_close`, `stop_mission` | `add`, `create`, `enqueue`, `insert_command`, `invalidate`, `publish`, `set_setting`, `stop_mission`, `stop_session`, `update_command` | parada/cierre |
| `RobotService.start_session` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1123) | 2 | Python / misión e historial | Medio; entrada/framework; síncrona | `_on_robot_message` | `create_session`, `publish` | sesión |
| `RobotService.stop_session` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1135) | 3 | Python / misión e historial | Medio; entrada/framework; síncrona | `_finish_close_preparation`, `_on_connection_state`, `_on_disconnect_grace_expired`, `_on_robot_message`, `close`, `send_command` | `publish`, `stop_mission` | sesión |
| `RobotService._cancel_disconnect_grace_timer` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1150) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_on_connection_state` | `cancel` | — |
| `RobotService._schedule_disconnect_grace` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1157) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_on_connection_state` | `start` | — |
| `RobotService._on_disconnect_grace_expired` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1169) | 1 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `insert_event`, `stop_session` | — |
| `RobotService._on_connection_state` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1178) | 6 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `_cancel_disconnect_grace_timer`, `_schedule_disconnect_grace`, `insert_event`, `invalidate`, `publish`, `stop_session`, `update_session_disconnect_reason` | — |
| `RobotService._on_command_sent` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1200) | 4 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `publish`, `update_command` | — |
| `RobotService._on_robot_message` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1209) | 43 | Python / misión e historial | Medio; sin llamada interna detectada; síncrona | — | `_advance_mission`, `_block_mission`, `_expire_stalled_mission`, `_on_step_settled`, `_persist_mission`, `_reconcile_short_memory_hello`, `_sync_surface_torque_from_telemetry`, `add`, `cancel`, `from_message`, `insert_event`, `invalidate`, `mission_status`, `public_dict`, `publish`, `send_command`, `snapshot`, `start`, `start_session`, `stop_mission`, `stop_session`, `submit`, `summary`, `telemetry_priority_signature`, `update_command`, `update_session_identity` | telemetría, parada/cierre |
| `RobotService._on_step_settled` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1388) | 11 | Python / misión e historial | Medio; interno; síncrona | `_on_robot_message` | `_advance_mission`, `_persist_mission`, `insert_event`, `mission_status`, `publish`, `send_command` | telemetría |
| `RobotService._reconcile_short_memory_hello` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1474) | 9 | Python / misión e historial | Medio; interno; síncrona | `_on_robot_message` | `_advance_mission`, `_block_mission`, `_queue_current_mission_step`, `_queue_final_alignment`, `insert_event`, `send_command`, `update_command` | — |
| `RobotService._movement_requires_stop` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1516) | 3 | Python / misión e historial | Bajo; interno; síncrona | `prepare_close` | — | telemetría |
| `RobotService._archive_mission_for_close` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1527) | 3 | Python / misión e historial | Medio; interno; síncrona | `_finish_close_preparation` | `_persist_mission`, `_set_return_state`, `cancel`, `mission_status`, `publish` | — |
| `RobotService._finish_close_preparation` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1546) | 7 | Python / misión e historial | Medio; interno; síncrona | `prepare_close` | `_archive_mission_for_close`, `fail_nonterminal_commands`, `insert_event`, `mission_status`, `stop_mission`, `stop_session` | — |
| `RobotService.prepare_close` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1577) | 11 | Python / misión e historial | Medio; interno; síncrona | `close_application` | `_finish_close_preparation`, `_movement_requires_stop`, `insert_event`, `send_command`, `snapshot` | — |
| `RobotService.purge_sessions` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1632) | 1 | Python / misión e historial | Bajo; interno; síncrona | `cleanup_sessions` | — | — |
| `RobotService._format_surface_dict` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1635) | 1 | Python / misión e historial | Bajo; interno; síncrona | `_sync_surface_torque_from_telemetry`, `apply_calibration_surface`, `get_calibration_surface`, `list_calibration_surfaces`, `save_calibration_surface` | — | — |
| `RobotService.list_calibration_surfaces` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1643) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_sync_surface_torque_from_telemetry`, `calibration_surfaces` | `_format_surface_dict` | — |
| `RobotService.get_calibration_surface` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1647) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_sync_surface_torque_from_telemetry`, `apply_calibration_surface`, `calibration_surface_item`, `save_calibration_surface` | `_format_surface_dict` | — |
| `RobotService.save_calibration_surface` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1651) | 5 | Python / misión e historial | Bajo; interno; síncrona | `_sync_surface_torque_from_telemetry`, `calibration_surfaces` | `_format_surface_dict`, `get_calibration_surface`, `publish` | — |
| `RobotService.delete_calibration_surface` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1676) | 2 | Python / misión e historial | Bajo; interno; síncrona | `calibration_surface_item` | `publish` | — |
| `RobotService.apply_calibration_surface` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1682) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_format_surface_dict`, `get_calibration_surface`, `send_command`, `set_setting` | — |
| `RobotService._sync_surface_torque_from_telemetry` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L1716) | 11 | Python / misión e historial | Bajo; interno; síncrona | `_on_robot_message` | `_format_surface_dict`, `get_calibration_surface`, `get_setting`, `list_calibration_surfaces`, `publish`, `save_calibration_surface` | — |
| `_service` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L24) | 1 | Python / misión e historial | Bajo; interno; síncrona | `acknowledge_open_area`, `apply_calibration_surface`, `calibration_surface_item`, `calibration_surfaces`, `cleanup_sessions`, `clear_robot_mission_memory`, `close_application`, `command`, `connection`, `delete_touch_recording`, `estop`, `events`, `export_session_json`, `export_telemetry`, `manual_drive`, `manual_start`, `manual_stop`, `missions`, `return_home`, `return_touch_recording`, `robot_config`, `sessions`, `start_session`, `status`, `stop_session`, `touch_recording`, `touch_recordings` | — | — |
| `_require_token` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L28) | 2 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `wrapper` | — |
| `_require_token.wrapper` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L30) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_require_token` | — | — |
| `index` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L39) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | — | — |
| `ui_asset` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L49) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | — | — |
| `status` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L57) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | `connection`, `events`, `stream` | `_service` | — |
| `robot_config` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L63) | 6 | Python / misión e historial | Bajo; entrada/framework; síncrona | `set_vectorial_routes_enabled` | `_service`, `set_robot_host`, `set_vectorial_routes_enabled` | — |
| `connection` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L82) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service`, `status` | — |
| `command` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L94) | 3 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service`, `send_command` | — |
| `estop` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L107) | 1 | Python / misión e historial | Medio; entrada/framework; síncrona | — | `_service`, `send_command` | parada/cierre |
| `manual_start` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L114) | 3 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service` | — |
| `manual_drive` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L125) | 3 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service` | — |
| `manual_stop` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L136) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service` | — |
| `touch_recordings` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L142) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | `initialize`, `save_touch_recording` | `_service` | — |
| `touch_recording` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L148) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | `acknowledge_open_area`, `return_touch_recording` | `_service` | — |
| `delete_touch_recording` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L157) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service` | — |
| `acknowledge_open_area` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L165) | 3 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service` | — |
| `return_touch_recording` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L176) | 3 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service` | — |
| `missions` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L187) | 5 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service`, `mission_status`, `start_mission`, `stop_mission` | — |
| `clear_robot_mission_memory` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L204) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service` | — |
| `return_home` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L210) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service`, `start_return_home` | — |
| `close_application` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L221) | 3 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service`, `prepare_close`, `start` | parada/cierre |
| `start_session` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L234) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | `_on_robot_message` | `_service` | — |
| `stop_session` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L240) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | `_finish_close_preparation`, `_on_connection_state`, `_on_disconnect_grace_expired`, `_on_robot_message`, `close`, `send_command` | `_service` | — |
| `cleanup_sessions` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L246) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service`, `purge_sessions` | — |
| `calibration_surfaces` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L259) | 4 | Python / misión e historial | Bajo; entrada/framework; síncrona | `initialize`, `save_calibration_surface` | `_service`, `list_calibration_surfaces`, `save_calibration_surface` | — |
| `calibration_surface_item` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L290) | 4 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service`, `delete_calibration_surface`, `get_calibration_surface` | — |
| `apply_calibration_surface` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L305) | 3 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service` | — |
| `export_telemetry` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L316) | 2 | Python / misión e historial | Medio; entrada/framework; síncrona | — | `_service`, `telemetry_rows` | telemetría, sesión |
| `sessions` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L333) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | `create_session` | `_service`, `session_rows` | — |
| `export_session_json` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L338) | 5 | Python / misión e historial | Medio; entrada/framework; síncrona | — | `_service`, `command_rows`, `event_rows`, `session_row`, `telemetry_rows` | telemetría, sesión |
| `events` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L356) | 3 | Python / misión e historial | Bajo; entrada/framework; cola/evento | `insert_event` | `_service`, `status`, `stream`, `subscribe`, `unsubscribe` | — |
| `events.stream` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L361) | 3 | Python / misión e historial | Bajo; interno; cola/evento | `events` | `status`, `unsubscribe` | — |
