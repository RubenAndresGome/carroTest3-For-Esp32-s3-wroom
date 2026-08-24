# UML funcional: `backend/python`

Funciones detectadas: **128**. Tipos detectados: **12**.

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
    n8019a4b26e["create_csp_nonce()"]
    n7502f63d12["secure_headers(response: Any)"]
  end
  subgraph f3["compaction.py"]
    n8d63ce830a["_quantize(value: Any, quantum: float)"]
    n332e428c8f["_normalized_payload(raw: str)"]
    n91f6dd4e52["clean(value: Any)"]
    nc654c3b31c["_telemetry_key(row: sqlite3.Row)"]
    n9ca71c4ce2["compact_database(path: Path, apply: bool = False, vacuum: bool = False)"]
  end
  subgraph f4["config.py"]
    n2c28dde60e["normalize_robot_host(value: object)"]
    n9e2127431d["from_environment(cls)"]
  end
  subgraph f5["database.py"]
    n6ce90dcc53["__init__(self, path: Path)"]
    n9d54627ca5["connect(self)"]
    n11d586ec95["transaction(self)"]
    nacb3ceafef["initialize(self)"]
    n18b8c76447["get_setting(self, key: str, default: Any = None)"]
    n47dbbcd128["set_setting(self, key: str, value: Any)"]
    nf7a3fd3fa7["create_session(self)"]
    n0bf80517b4["update_session_identity(self, session_id: int, robot_id: str | None, firmware_version: str | None, protocol: str | None)"]
    n60b0cf8cad["stop_session(self, session_id: int, reason: str | None = None)"]
    naf02b1bc78["insert_command(self, command_id: str, session_id: int | None, name: str, payload: dict[str, Any], status: str)"]
    n6541552cc2["update_command(self, command_id: str, status: str, error: str | None = None)"]
    nf41c46053c["fail_nonterminal_commands(self, reason: str)"]
    n3a66901c11["close_orphan_sessions(self, reason: str)"]
    n199de97a81["insert_event(self, session_id: int | None, kind: str, severity: str, payload: dict[str, Any])"]
    n52da51b66e["_quantize(value: Any, quantum: float)"]
    nbc806381e8["telemetry_priority_signature(cls, snapshot: Any)"]
    n9bd103dfd7["telemetry_fingerprint(cls, snapshot: Any)"]
    n070eea3cc7["_compact_payload(snapshot: Any)"]
    nc8500bc751["insert_telemetry(self, session_id: int, snapshot: Any)"]
    n5f84836d20["telemetry_rows(self, session_id: int)"]
    n2030e2bd2f["session_rows(self)"]
    n272da64f6c["session_row(self, session_id: int)"]
    nc44d70c1da["command_rows(self, session_id: int)"]
    ndbedf3f906["event_rows(self, session_id: int)"]
    n8106c54b68["purge_sessions(self, days: int)"]
    n62c2af3d19["optimize_storage(self, full: bool = False)"]
  end
  subgraph f6["domain.py"]
    nb983649585["_finite_number(value: Any, name: str, low: float, high: float)"]
    n5dae047b8f["_heading_degrees(value: Any)"]
    n8e2fbfdb5d["validate_command_payload(name: str, payload: Mapping[str, Any] | None)"]
    n67c3bdbb31["create(cls, name: object, payload: Mapping[str, Any] | None = None, seq: int = 0, command_id: str | None = None)"]
    nede5d2494e["protocol_envelope(self)"]
    n918003cd14["split_segment_mm( start_x_mm: float, start_y_mm: float, end_x_mm: float, end_y_mm: float, max_segment_mm: float = MAX_SEGMENT_MM, )"]
    n51981cbeb3["from_message(cls, message: Mapping[str, Any], fallback_sequence: int)"]
    n46775125a7["public_dict(self)"]
  end
  subgraph f7["gateway.py"]
    n768722c274["__init__( self, host_getter: Callable[[], str], on_message: Callable[[dict[str, Any]], None], on_state: Callable[[ConnectionState, str | None], None], on_sent: Callable[[RobotCommand], None], session_getter: Callable[[], str] = lambda: '', max_message_bytes: int = 4096, )"]
    n9c7f87a274["start(self)"]
    n06e0517455["stop(self)"]
    n7e9b9be06c["request_reconnect(self)"]
    ne1b1b4ed58["enqueue(self, command: RobotCommand)"]
    nd7487f13c1["cancel(self, command_id: str | None)"]
    nc197fe04ab["snapshot(self)"]
    n75cdaae440["_set_state(self, state: ConnectionState, detail: str | None)"]
    n1520d57fd8["_run(self)"]
    n5a1cf90c3e["_drain_one(self, connection: Any)"]
    n4aa436b2b5["_receive(self, raw: str | bytes)"]
  end
  subgraph f8["services.py"]
    nc58992ce45["__init__(self, subscriber_size: int = 64)"]
    na9729b1c67["subscribe(self)"]
    n88563040de["unsubscribe(self, subscriber: queue.Queue[str])"]
    n4c576330b1["publish(self, kind: str, payload: dict[str, Any])"]
    n34e81b855a["__init__(self, database: Database, queue_size: int = 256)"]
    n55583f40a2["submit(self, session_id: int, snapshot: TelemetrySnapshot)"]
    nf78d3ee8b1["stop(self)"]
    n76f6a478db["_run(self)"]
    n21b7e67766["__init__(self, database: Database, start_gateway: bool = True, max_message_bytes: int = 4096)"]
    nd7c4778c17["close(self)"]
    n1d227e920d["get_robot_host(self)"]
    n2837494ee6["set_robot_host(self, value: object)"]
    n1957ce84e6["connect(self)"]
    n7c02a4d30d["disconnect(self)"]
    n7dc9552171["reconnect(self)"]
    n97af93f1c6["status(self)"]
    ne78aa088df["mission_status(self)"]
    ne007ec69a2["_persist_mission(self)"]
    n25d658f999["_mission_wire_points(self)"]
    nf1aeb31b3d["_require_ready_robot(self)"]
    naaf08a29e6["_build_segments(self, points: list[Any], mission_origin: dict[str, float])"]
    n50fa861f35["_activate_mission(self, segments: list[dict[str, float]], mission_origin: dict[str, float], kind: str, final_heading: float | None = None)"]
    nc72c80f198["start_mission(self, points: object)"]
    n46a3c3f292["start_return_home(self)"]
    n7401223799["stop_mission(self, reason: str = 'operator_stop')"]
    n076a4f2906["clear_robot_mission_memory(self)"]
    n65629473d0["_advance_mission(self)"]
    n66d0abd223["_set_return_state(self, state: str, error: str | None = None)"]
    n94c7e22137["_queue_current_mission_step(self, seq_override: int | None = None)"]
    needc821158["_queue_final_alignment(self, seq_override: int | None = None)"]
    n024ecb3a08["_expire_stalled_mission(self)"]
    n3e6b3beaca["_block_mission(self, reason: str)"]
    n2087b5efc4["_reconcile_mission_snapshot(self, snapshot: TelemetrySnapshot)"]
    n9153356e4f["_reconcile_firmware_mission(self, firmware: dict[str, Any])"]
    n81d8816604["send_command(self, name: object, payload: dict[str, Any] | None, seq_override: int | None = None, command_id_override: str | None = None)"]
    n1eec8828b6["start_session(self)"]
    n4958fad6d4["stop_session(self, reason: str | None = None)"]
    nbb05040b56["_on_connection_state(self, state: ConnectionState, detail: str | None)"]
    nff78a2797d["_on_command_sent(self, command: RobotCommand)"]
    n9a57989029["_on_robot_message(self, message: dict[str, Any])"]
    na32a4ec87e["_reconcile_short_memory_hello(self, message: dict[str, Any])"]
    nc0689735c6["_movement_requires_stop(self)"]
    n1b57255582["_archive_mission_for_close(self)"]
    n517df21a27["_finish_close_preparation(self, force: bool, stop_required: bool, stop_confirmed: bool)"]
    n62f8d1d95e["prepare_close(self, force: bool = False, timeout_s: float | None = None)"]
    nb45f7f7fe9["purge_sessions(self, days: int)"]
  end
  subgraph f9["web.py"]
    n11a7fd3b87["_service()"]
    n9768753665["_require_token(function: Callable[..., Any])"]
    nab39be2bb6["wrapper(*args: Any, **kwargs: Any)"]
    nef186c8e4b["index()"]
    nda183cb233["ui_asset(filename: str)"]
    n815823909d["status()"]
    n1eb416b959["robot_config()"]
    n73c8d4811c["connection(action: str)"]
    n93a37929af["command()"]
    nd45a072485["estop()"]
    n8fdba82311["missions()"]
    n69f79285a5["clear_robot_mission_memory()"]
    na13c11dcdf["return_home()"]
    n18b113cef4["close_application()"]
    n5f28d62235["start_session()"]
    n1ef8b6f8a4["stop_session()"]
    na5e8d26bb3["cleanup_sessions()"]
    n13eb31c6b3["export_telemetry(session_id: int)"]
    n4316fdd446["sessions()"]
    n8e8dc327c9["export_session_json(session_id: int)"]
    n67714987c3["events()"]
    n25871ada17["stream()"]
  end
  n024ecb3a08 --> n3e6b3beaca
  n06e0517455 --> n75cdaae440
  n076a4f2906 --> n7401223799
  n0bf80517b4 --> n11d586ec95
  n11d586ec95 --> n9d54627ca5
  n11d586ec95 --> nd7c4778c17
  n13eb31c6b3 --> n11a7fd3b87
  n13eb31c6b3 --> n5f84836d20
  n1520d57fd8 --> n4aa436b2b5
  n1520d57fd8 --> n5a1cf90c3e
  n1520d57fd8 --> n75cdaae440
  n1520d57fd8 --> nd7c4778c17
  n18b113cef4 --> n11a7fd3b87
  n18b113cef4 --> n62f8d1d95e
  n18b113cef4 --> n9c7f87a274
  n18b8c76447 --> n9d54627ca5
  n1957ce84e6 --> n9c7f87a274
  n199de97a81 --> n11d586ec95
  n199de97a81 --> n67714987c3
  n1b57255582 --> n4c576330b1
  n1b57255582 --> n66d0abd223
  n1b57255582 --> nd7487f13c1
  n1b57255582 --> ne007ec69a2
  n1b57255582 --> ne78aa088df
  n1eb416b959 --> n11a7fd3b87
  n1eb416b959 --> n1d227e920d
  n1eb416b959 --> n2837494ee6
  n1eec8828b6 --> n4c576330b1
  n1eec8828b6 --> nf7a3fd3fa7
  n1ef8b6f8a4 --> n11a7fd3b87
  n2030e2bd2f --> n9d54627ca5
  n2087b5efc4 --> n3e6b3beaca
  n2087b5efc4 --> n9153356e4f
  n21b7e67766 --> n18b8c76447
  n21b7e67766 --> n199de97a81
  n21b7e67766 --> n2c28dde60e
  n21b7e67766 --> n3a66901c11
  n21b7e67766 --> n47dbbcd128
  n21b7e67766 --> n9c7f87a274
  n21b7e67766 --> ne007ec69a2
  n21b7e67766 --> nf41c46053c
  n25871ada17 -.-> n88563040de
  n25871ada17 -.-> n97af93f1c6
  n272da64f6c --> n9d54627ca5
  n2837494ee6 --> n2c28dde60e
  n2837494ee6 --> n47dbbcd128
  n2837494ee6 --> n4c576330b1
  n2837494ee6 --> n7e9b9be06c
  n332e428c8f --> n91f6dd4e52
  n34e81b855a -.-> n9c7f87a274
  n3a66901c11 --> n11d586ec95
  n3e6b3beaca --> n4c576330b1
  n3e6b3beaca --> n6541552cc2
  n3e6b3beaca --> n66d0abd223
  n3e6b3beaca --> n81d8816604
  n3e6b3beaca --> nd7487f13c1
  n3e6b3beaca --> ne007ec69a2
  n3e6b3beaca --> ne78aa088df
  n3e89fea1ba --> nd7c4778c17
  n4316fdd446 --> n11a7fd3b87
  n4316fdd446 --> n2030e2bd2f
  n46a3c3f292 --> n18b8c76447
  n46a3c3f292 --> n47dbbcd128
  n46a3c3f292 --> n50fa861f35
  n46a3c3f292 --> nf1aeb31b3d
  n47dbbcd128 --> n11d586ec95
  n4958fad6d4 --> n4c576330b1
  n50fa861f35 --> n18b8c76447
  n50fa861f35 --> n47dbbcd128
  n50fa861f35 --> n4c576330b1
  n50fa861f35 --> n94c7e22137
  n50fa861f35 --> ne007ec69a2
  n50fa861f35 --> ne78aa088df
  n517df21a27 --> n199de97a81
  n517df21a27 --> n1b57255582
  n517df21a27 --> n60b0cf8cad
  n517df21a27 --> ne78aa088df
  n517df21a27 --> nf41c46053c
  n51981cbeb3 --> n5dae047b8f
  n51981cbeb3 --> nb983649585
  n5a1cf90c3e -.-> nede5d2494e
  n5dae047b8f --> nb983649585
  n5f28d62235 --> n11a7fd3b87
  n5f84836d20 --> n9d54627ca5
  n60b0cf8cad --> n11d586ec95
  n62c2af3d19 --> n9d54627ca5
  n62c2af3d19 --> nd7c4778c17
  n62f8d1d95e --> n199de97a81
  n62f8d1d95e --> n517df21a27
  n62f8d1d95e --> n81d8816604
  n62f8d1d95e --> nc0689735c6
  n62f8d1d95e --> nc197fe04ab
  n6541552cc2 --> n11d586ec95
  n65629473d0 --> n47dbbcd128
  n65629473d0 --> n4c576330b1
  n65629473d0 --> n66d0abd223
  n65629473d0 --> n94c7e22137
  n65629473d0 --> ne007ec69a2
  n65629473d0 --> ne78aa088df
  n65629473d0 --> needc821158
  n66d0abd223 --> n18b8c76447
  n66d0abd223 --> n47dbbcd128
  n67714987c3 -.-> n11a7fd3b87
  n67714987c3 -.-> n25871ada17
  n67714987c3 -.-> n88563040de
  n67714987c3 -.-> n97af93f1c6
  n67714987c3 -.-> na9729b1c67
  n67c3bdbb31 --> n8e2fbfdb5d
  n69f79285a5 --> n11a7fd3b87
  n73c8d4811c --> n11a7fd3b87
  n73c8d4811c --> n97af93f1c6
  n7401223799 --> n18b8c76447
  n7401223799 --> n47dbbcd128
  n7401223799 --> n4c576330b1
  n7401223799 --> n6541552cc2
  n7401223799 --> n66d0abd223
  n7401223799 --> n81d8816604
  n7401223799 --> nd7487f13c1
  n7401223799 --> ne007ec69a2
  n7401223799 --> ne78aa088df
  n76f6a478db -.-> nc8500bc751
  n7c02a4d30d --> n06e0517455
  n7dc9552171 --> n7e9b9be06c
  n7dc9552171 --> n9c7f87a274
  n7dc9552171 --> nc197fe04ab
  n8106c54b68 --> n11d586ec95
  n815823909d --> n11a7fd3b87
  n81d8816604 --> n47dbbcd128
  n81d8816604 --> n4c576330b1
  n81d8816604 --> n6541552cc2
  n81d8816604 --> n67c3bdbb31
  n81d8816604 --> naf02b1bc78
  n81d8816604 --> ne1b1b4ed58
  n8e2fbfdb5d --> n5dae047b8f
  n8e2fbfdb5d --> nb983649585
  n8e8dc327c9 --> n11a7fd3b87
  n8e8dc327c9 --> n272da64f6c
  n8e8dc327c9 --> n5f84836d20
  n8e8dc327c9 --> nc44d70c1da
  n8e8dc327c9 --> ndbedf3f906
  n8fdba82311 --> n11a7fd3b87
  n8fdba82311 --> n7401223799
  n8fdba82311 --> nc72c80f198
  n8fdba82311 --> ne78aa088df
  n9153356e4f --> n4c576330b1
  n9153356e4f --> ne007ec69a2
  n9153356e4f --> ne78aa088df
  n918003cd14 --> nb983649585
  n9222a02d25 --> n3e89fea1ba
  n9222a02d25 --> n4303201550
  n9222a02d25 --> n9c7f87a274
  n9222a02d25 --> nd5db088795
  n9222a02d25 --> nd7c4778c17
  n9222a02d25 --> nfb667eae25
  n93a37929af --> n11a7fd3b87
  n93a37929af --> n81d8816604
  n94c7e22137 --> n81d8816604
  n94c7e22137 --> ne007ec69a2
  n9768753665 --> nab39be2bb6
  n97af93f1c6 --> n46775125a7
  n97af93f1c6 --> nc197fe04ab
  n97af93f1c6 --> ne78aa088df
  n9a57989029 --> n024ecb3a08
  n9a57989029 --> n0bf80517b4
  n9a57989029 --> n199de97a81
  n9a57989029 --> n3e6b3beaca
  n9a57989029 --> n46775125a7
  n9a57989029 --> n4c576330b1
  n9a57989029 --> n51981cbeb3
  n9a57989029 --> n55583f40a2
  n9a57989029 --> n6541552cc2
  n9a57989029 --> n65629473d0
  n9a57989029 --> na32a4ec87e
  n9a57989029 --> nbc806381e8
  n9a57989029 --> nc197fe04ab
  n9bd103dfd7 --> n8d63ce830a
  n9bd103dfd7 --> nbc806381e8
  n9c7f87a274 --> nc197fe04ab
  n9ca71c4ce2 --> n332e428c8f
  n9ca71c4ce2 --> n62c2af3d19
  n9ca71c4ce2 --> n9d54627ca5
  n9ca71c4ce2 --> nacb3ceafef
  n9ca71c4ce2 --> nc654c3b31c
  n9ca71c4ce2 --> nd7c4778c17
  n9e2127431d --> n2c28dde60e
  na13c11dcdf --> n11a7fd3b87
  na13c11dcdf --> n46a3c3f292
  na32a4ec87e --> n199de97a81
  na32a4ec87e --> n3e6b3beaca
  na32a4ec87e --> n6541552cc2
  na32a4ec87e --> n65629473d0
  na32a4ec87e --> n81d8816604
  na32a4ec87e --> n94c7e22137
  na32a4ec87e --> needc821158
  na5e8d26bb3 --> n11a7fd3b87
  na5e8d26bb3 --> n8106c54b68
  naaf08a29e6 --> n67c3bdbb31
  naaf08a29e6 --> n918003cd14
  nacb3ceafef --> n9d54627ca5
  nacb3ceafef --> nd7c4778c17
  naf02b1bc78 --> n11d586ec95
  nbb05040b56 --> n199de97a81
  nbb05040b56 --> n1eec8828b6
  nbb05040b56 --> n4c576330b1
  nbb05040b56 --> n60b0cf8cad
  nc44d70c1da --> n9d54627ca5
  nc654c3b31c --> n332e428c8f
  nc654c3b31c --> n8d63ce830a
  nc72c80f198 --> n50fa861f35
  nc72c80f198 --> naaf08a29e6
  nc72c80f198 --> nf1aeb31b3d
  nc8500bc751 --> n070eea3cc7
  nc8500bc751 --> n11d586ec95
  nc8500bc751 --> n9bd103dfd7
  nd45a072485 --> n11a7fd3b87
  nd45a072485 --> n81d8816604
  nd7c4778c17 --> n06e0517455
  nd7c4778c17 --> n60b0cf8cad
  ndbedf3f906 --> n9d54627ca5
  ne007ec69a2 --> n47dbbcd128
  needc821158 --> n81d8816604
  needc821158 --> ne007ec69a2
  nf1aeb31b3d --> nc197fe04ab
  nf41c46053c --> n11d586ec95
  nf7a3fd3fa7 --> n11d586ec95
  nf7a3fd3fa7 --> n4316fdd446
  nfb676358e5 --> n18b8c76447
  nfb676358e5 --> n47dbbcd128
  nfb676358e5 --> n7502f63d12
  nfb676358e5 --> n8019a4b26e
  nfb676358e5 --> n9e2127431d
  nfb676358e5 --> nacb3ceafef
  nff78a2797d --> n4c576330b1
  nff78a2797d --> n6541552cc2
  classDef alto fill:#5b1f2a,stroke:#ff7a7a,color:#fff
  classDef medio fill:#4a3717,stroke:#ffca67,color:#fff
  classDef bajo fill:#123b3a,stroke:#39e6aa,color:#fff
  class n4303201550 bajo
  class nd5db088795 bajo
  class n9222a02d25 bajo
  class n3e89fea1ba bajo
  class nfb667eae25 bajo
  class nfb676358e5 bajo
  class n8019a4b26e bajo
  class n7502f63d12 bajo
  class n8d63ce830a bajo
  class n332e428c8f bajo
  class n91f6dd4e52 bajo
  class nc654c3b31c bajo
  class n9ca71c4ce2 medio
  class n2c28dde60e bajo
  class n9e2127431d medio
  class n6ce90dcc53 bajo
  class n9d54627ca5 bajo
  class n11d586ec95 bajo
  class nacb3ceafef bajo
  class n18b8c76447 bajo
  class n47dbbcd128 bajo
  class nf7a3fd3fa7 bajo
  class n0bf80517b4 bajo
  class n60b0cf8cad bajo
  class naf02b1bc78 bajo
  class n6541552cc2 bajo
  class nf41c46053c bajo
  class n3a66901c11 bajo
  class n199de97a81 medio
  class n52da51b66e bajo
  class nbc806381e8 bajo
  class n9bd103dfd7 bajo
  class n070eea3cc7 bajo
  class nc8500bc751 bajo
  class n5f84836d20 bajo
  class n2030e2bd2f bajo
  class n272da64f6c bajo
  class nc44d70c1da bajo
  class ndbedf3f906 bajo
  class n8106c54b68 bajo
  class n62c2af3d19 bajo
  class nb983649585 bajo
  class n5dae047b8f bajo
  class n8e2fbfdb5d bajo
  class n67c3bdbb31 bajo
  class nede5d2494e bajo
  class n918003cd14 bajo
  class n51981cbeb3 medio
  class n46775125a7 medio
  class n768722c274 bajo
  class n9c7f87a274 medio
  class n06e0517455 medio
  class n7e9b9be06c bajo
  class ne1b1b4ed58 medio
  class nd7487f13c1 bajo
  class nc197fe04ab bajo
  class n75cdaae440 medio
  class n1520d57fd8 medio
  class n5a1cf90c3e bajo
  class n4aa436b2b5 medio
  class nc58992ce45 bajo
  class na9729b1c67 bajo
  class n88563040de bajo
  class n4c576330b1 bajo
  class n34e81b855a medio
  class n55583f40a2 bajo
  class nf78d3ee8b1 medio
  class n76f6a478db bajo
  class n21b7e67766 bajo
  class nd7c4778c17 medio
  class n1d227e920d bajo
  class n2837494ee6 bajo
  class n1957ce84e6 bajo
  class n7c02a4d30d medio
  class n7dc9552171 bajo
  class n97af93f1c6 medio
  class ne78aa088df bajo
  class ne007ec69a2 bajo
  class n25d658f999 bajo
  class nf1aeb31b3d bajo
  class naaf08a29e6 bajo
  class n50fa861f35 medio
  class nc72c80f198 bajo
  class n46a3c3f292 bajo
  class n7401223799 medio
  class n076a4f2906 bajo
  class n65629473d0 medio
  class n66d0abd223 bajo
  class n94c7e22137 bajo
  class needc821158 bajo
  class n024ecb3a08 bajo
  class n3e6b3beaca medio
  class n2087b5efc4 medio
  class n9153356e4f medio
  class n81d8816604 medio
  class n1eec8828b6 medio
  class n4958fad6d4 medio
  class nbb05040b56 bajo
  class nff78a2797d bajo
  class n9a57989029 medio
  class na32a4ec87e medio
  class nc0689735c6 bajo
  class n1b57255582 medio
  class n517df21a27 medio
  class n62f8d1d95e medio
  class nb45f7f7fe9 bajo
  class n11a7fd3b87 bajo
  class n9768753665 bajo
  class nab39be2bb6 bajo
  class nef186c8e4b bajo
  class nda183cb233 bajo
  class n815823909d bajo
  class n1eb416b959 bajo
  class n73c8d4811c bajo
  class n93a37929af bajo
  class nd45a072485 medio
  class n8fdba82311 bajo
  class n69f79285a5 bajo
  class na13c11dcdf bajo
  class n18b113cef4 bajo
  class n5f28d62235 bajo
  class n1ef8b6f8a4 bajo
  class na5e8d26bb3 bajo
  class n13eb31c6b3 medio
  class n4316fdd446 bajo
  class n8e8dc327c9 medio
  class n67714987c3 bajo
  class n25871ada17 bajo
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
| `create_app.create_csp_nonce` | [`desktop_app/robot_app/app_factory.py`](../../desktop_app/robot_app/app_factory.py#L36) | 1 | Python / misión e historial | Bajo; interno; síncrona | `create_app` | — | — |
| `create_app.secure_headers` | [`desktop_app/robot_app/app_factory.py`](../../desktop_app/robot_app/app_factory.py#L40) | 1 | Python / misión e historial | Bajo; interno; síncrona | `create_app` | — | — |
| `_quantize` | [`desktop_app/robot_app/compaction.py`](../../desktop_app/robot_app/compaction.py#L15) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_telemetry_key`, `telemetry_fingerprint` | — | — |
| `_normalized_payload` | [`desktop_app/robot_app/compaction.py`](../../desktop_app/robot_app/compaction.py#L22) | 8 | Python / misión e historial | Bajo; interno; síncrona | `_telemetry_key`, `compact_database` | `clean` | — |
| `_normalized_payload.clean` | [`desktop_app/robot_app/compaction.py`](../../desktop_app/robot_app/compaction.py#L33) | 7 | Python / misión e historial | Bajo; interno; síncrona | `_normalized_payload` | — | — |
| `_telemetry_key` | [`desktop_app/robot_app/compaction.py`](../../desktop_app/robot_app/compaction.py#L45) | 2 | Python / misión e historial | Bajo; interno; síncrona | `compact_database` | `_normalized_payload`, `_quantize` | — |
| `compact_database` | [`desktop_app/robot_app/compaction.py`](../../desktop_app/robot_app/compaction.py#L60) | 32 | Python / misión e historial | Medio; sin llamada interna detectada; síncrona | — | `_normalized_payload`, `_telemetry_key`, `close`, `connect`, `initialize`, `optimize_storage`, `update` | telemetría, parada/cierre |
| `normalize_robot_host` | [`desktop_app/robot_app/config.py`](../../desktop_app/robot_app/config.py#L16) | 6 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `from_environment`, `set_robot_host` | — | — |
| `AppConfig.from_environment` | [`desktop_app/robot_app/config.py`](../../desktop_app/robot_app/config.py#L39) | 1 | Python / misión e historial | Medio; interno; asíncrona | `create_app` | `normalize_robot_host` | — |
| `Database.__init__` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L14) | 1 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Database.connect` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L18) | 1 | Python / misión e historial | Bajo; interno; síncrona | `command_rows`, `compact_database`, `connect_db`, `event_rows`, `get_setting`, `initialize`, `main`, `optimize_storage`, `session_row`, `session_rows`, `telemetry_rows`, `transaction` | — | — |
| `Database.transaction` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L28) | 3 | Python / misión e historial | Bajo; interno; síncrona | `close_orphan_sessions`, `create_session`, `fail_nonterminal_commands`, `insert_command`, `insert_event`, `insert_telemetry`, `purge_sessions`, `set_setting`, `stop_session`, `update_command`, `update_session_identity` | `close`, `connect` | — |
| `Database.initialize` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L41) | 5 | Python / misión e historial | Bajo; interno; síncrona | `compact_database`, `create_app` | `close`, `connect` | telemetría |
| `Database.get_setting` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L76) | 3 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `_activate_mission`, `_set_return_state`, `create_app`, `start_return_home`, `stop_mission` | `connect` | parada/cierre |
| `Database.set_setting` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L81) | 3 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `_activate_mission`, `_advance_mission`, `_persist_mission`, `_set_return_state`, `create_app`, `send_command`, `set_robot_host`, `start_return_home`, `stop_mission` | `transaction` | — |
| `Database.create_session` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L90) | 1 | Python / misión e historial | Bajo; interno; síncrona | `start_session` | `sessions`, `transaction` | — |
| `Database.update_session_identity` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L95) | 5 | Python / misión e historial | Bajo; interno; síncrona | `_on_robot_message` | `transaction` | — |
| `Database.stop_session` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L102) | 3 | Python / misión e historial | Bajo; entrada/framework; síncrona | `_finish_close_preparation`, `_on_connection_state`, `close` | `transaction` | — |
| `Database.insert_command` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L109) | 6 | Python / misión e historial | Bajo; interno; síncrona | `send_command` | `transaction` | — |
| `Database.update_command` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L116) | 6 | Python / misión e historial | Bajo; interno; síncrona | `_block_mission`, `_on_command_sent`, `_on_robot_message`, `_reconcile_short_memory_hello`, `send_command`, `stop_mission` | `transaction` | — |
| `Database.fail_nonterminal_commands` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L128) | 2 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `_finish_close_preparation` | `transaction` | — |
| `Database.close_orphan_sessions` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L138) | 2 | Python / misión e historial | Bajo; interno; síncrona | `__init__` | `transaction` | — |
| `Database.insert_event` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L146) | 17 | Python / misión e historial | Medio; interno; síncrona | `__init__`, `_finish_close_preparation`, `_on_connection_state`, `_on_robot_message`, `_reconcile_short_memory_hello`, `prepare_close` | `events`, `transaction` | parada/cierre |
| `Database._quantize` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L191) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_telemetry_key`, `telemetry_fingerprint` | — | — |
| `Database.telemetry_priority_signature` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L198) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_on_robot_message`, `telemetry_fingerprint` | — | — |
| `Database.telemetry_fingerprint` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L216) | 3 | Python / misión e historial | Bajo; interno; síncrona | `insert_telemetry` | `_quantize`, `telemetry_priority_signature` | — |
| `Database._compact_payload` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L233) | 5 | Python / misión e historial | Bajo; interno; síncrona | `insert_telemetry` | — | telemetría |
| `Database.insert_telemetry` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L250) | 30 | Python / misión e historial | Bajo; interno; síncrona | `_run` | `_compact_payload`, `telemetry_fingerprint`, `transaction` | telemetría |
| `Database.telemetry_rows` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L281) | 2 | Python / misión e historial | Bajo; interno; síncrona | `export_session_json`, `export_telemetry` | `connect` | telemetría, parada/cierre |
| `Database.session_rows` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L292) | 1 | Python / misión e historial | Bajo; interno; síncrona | `sessions` | `connect` | telemetría, parada/cierre |
| `Database.session_row` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L304) | 2 | Python / misión e historial | Bajo; interno; síncrona | `export_session_json` | `connect` | parada/cierre |
| `Database.command_rows` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L308) | 2 | Python / misión e historial | Bajo; interno; síncrona | `export_session_json` | `connect` | parada/cierre |
| `Database.event_rows` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L315) | 2 | Python / misión e historial | Bajo; interno; síncrona | `export_session_json` | `connect` | parada/cierre |
| `Database.purge_sessions` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L324) | 9 | Python / misión e historial | Bajo; interno; síncrona | `cleanup_sessions` | `transaction` | telemetría |
| `Database.optimize_storage` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L345) | 4 | Python / misión e historial | Bajo; interno; síncrona | `compact_database` | `close`, `connect` | — |
| `_finite_number` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L45) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_heading_degrees`, `from_message`, `split_segment_mm`, `validate_command_payload` | — | — |
| `_heading_degrees` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L54) | 1 | Python / misión e historial | Bajo; interno; síncrona | `from_message`, `validate_command_payload` | `_finite_number` | — |
| `validate_command_payload` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L58) | 10 | Python / misión e historial | Bajo; interno; síncrona | `create` | `_finite_number`, `_heading_degrees` | — |
| `RobotCommand.create` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L101) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_build_segments`, `send_command` | `validate_command_payload` | — |
| `RobotCommand.protocol_envelope` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L115) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_drain_one` | — | — |
| `split_segment_mm` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L121) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_build_segments` | `_finite_number` | — |
| `TelemetrySnapshot.from_message` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L243) | 38 | Python / misión e historial | Medio; interno; síncrona | `_on_robot_message` | `_finite_number`, `_heading_degrees` | telemetría |
| `TelemetrySnapshot.public_dict` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L414) | 1 | Python / misión e historial | Medio; interno; síncrona | `_on_robot_message`, `status` | — | — |
| `RobotGateway.__init__` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L37) | 1 | Python / misión e historial | Bajo; sin llamada interna detectada; cola/evento | — | — | — |
| `RobotGateway.start` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L67) | 3 | Python / misión e historial | Medio; interno; asíncrona | `__init__`, `close_application`, `connect`, `init`, `main`, `reconnect` | `clear`, `snapshot` | — |
| `RobotGateway.stop` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L77) | 2 | Python / misión e historial | Medio; interno; síncrona | `close`, `disconnect` | `_set_state` | — |
| `RobotGateway.request_reconnect` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L84) | 2 | Python / misión e historial | Bajo; interno; síncrona | `reconnect`, `set_robot_host` | — | — |
| `RobotGateway.enqueue` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L88) | 4 | Python / misión e historial | Medio; interno; cola/evento | `send_command` | — | parada/cierre |
| `RobotGateway.cancel` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L96) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_archive_mission_for_close`, `_block_mission`, `stop_mission` | `add` | — |
| `RobotGateway.snapshot` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L101) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_on_robot_message`, `_require_ready_robot`, `prepare_close`, `reconnect`, `start`, `status` | — | — |
| `RobotGateway._set_state` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L117) | 3 | Python / misión e historial | Medio; interno; asíncrona | `_run`, `stop` | — | — |
| `RobotGateway._run` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L124) | 14 | Python / misión e historial | Medio; sin llamada interna detectada; asíncrona | — | `_drain_one`, `_receive`, `_set_state`, `clear`, `close`, `send` | sesión |
| `RobotGateway._drain_one` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L202) | 7 | Python / misión e historial | Bajo; interno; cola/evento | `_run` | `protocol_envelope`, `send` | — |
| `RobotGateway._receive` | [`desktop_app/robot_app/gateway.py`](../../desktop_app/robot_app/gateway.py#L224) | 8 | Python / misión e historial | Medio; interno; síncrona | `_run` | — | sesión |
| `EventHub.__init__` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L21) | 1 | Python / misión e historial | Bajo; sin llamada interna detectada; cola/evento | — | — | — |
| `EventHub.subscribe` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L26) | 1 | Python / misión e historial | Bajo; interno; cola/evento | `events` | `add` | — |
| `EventHub.unsubscribe` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L32) | 1 | Python / misión e historial | Bajo; interno; cola/evento | `events`, `stream` | — | — |
| `EventHub.publish` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L36) | 4 | Python / misión e historial | Bajo; interno; cola/evento | `_activate_mission`, `_advance_mission`, `_archive_mission_for_close`, `_block_mission`, `_on_command_sent`, `_on_connection_state`, `_on_robot_message`, `_reconcile_firmware_mission`, `send_command`, `set_robot_host`, `start_session`, `stop_mission`, `stop_session` | — | — |
| `TelemetryRecorder.__init__` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L52) | 1 | Python / misión e historial | Medio; sin llamada interna detectada; cola/evento | — | `start` | telemetría |
| `TelemetryRecorder.submit` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L60) | 3 | Python / misión e historial | Bajo; interno; cola/evento | `_on_robot_message` | — | — |
| `TelemetryRecorder.stop` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L70) | 1 | Python / misión e historial | Medio; interno; síncrona | `close`, `disconnect` | — | — |
| `TelemetryRecorder._run` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L74) | 4 | Python / misión e historial | Bajo; sin llamada interna detectada; cola/evento | — | `insert_telemetry` | — |
| `RobotService.__init__` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L94) | 11 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `_persist_mission`, `close_orphan_sessions`, `fail_nonterminal_commands`, `get_setting`, `insert_event`, `normalize_robot_host`, `set_setting`, `start` | misión activa |
| `RobotService.close` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L191) | 3 | Python / misión e historial | Medio; interno; síncrona | `_run`, `closeApplication`, `compact_database`, `initialize`, `main`, `optimize_storage`, `run`, `shutdown_host`, `stop`, `transaction` | `stop`, `stop_session` | — |
| `RobotService.get_robot_host` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L203) | 1 | Python / misión e historial | Bajo; interno; síncrona | `robot_config` | — | — |
| `RobotService.set_robot_host` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L207) | 2 | Python / misión e historial | Bajo; interno; síncrona | `robot_config` | `normalize_robot_host`, `publish`, `request_reconnect`, `set_setting` | — |
| `RobotService.connect` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L218) | 1 | Python / misión e historial | Bajo; interno; síncrona | `command_rows`, `compact_database`, `connect_db`, `event_rows`, `get_setting`, `initialize`, `main`, `optimize_storage`, `session_row`, `session_rows`, `telemetry_rows`, `transaction` | `start` | — |
| `RobotService.disconnect` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L221) | 1 | Python / misión e historial | Medio; sin llamada interna detectada; síncrona | — | `stop` | — |
| `RobotService.reconnect` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L224) | 2 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `request_reconnect`, `snapshot`, `start` | — |
| `RobotService.status` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L230) | 2 | Python / misión e historial | Medio; entrada/framework; síncrona | `connection`, `events`, `stream` | `mission_status`, `public_dict`, `snapshot` | telemetría, parada/cierre |
| `RobotService.mission_status` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L248) | 4 | Python / misión e historial | Bajo; interno; síncrona | `_activate_mission`, `_advance_mission`, `_archive_mission_for_close`, `_block_mission`, `_finish_close_preparation`, `_reconcile_firmware_mission`, `missions`, `status`, `stop_mission` | — | — |
| `RobotService._persist_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L270) | 2 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `_activate_mission`, `_advance_mission`, `_archive_mission_for_close`, `_block_mission`, `_queue_current_mission_step`, `_queue_final_alignment`, `_reconcile_firmware_mission`, `stop_mission` | `set_setting` | misión activa |
| `RobotService._mission_wire_points` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L283) | 2 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `RobotService._require_ready_robot` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L292) | 7 | Python / misión e historial | Bajo; interno; síncrona | `start_mission`, `start_return_home` | `snapshot` | estado, telemetría |
| `RobotService._build_segments` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L310) | 9 | Python / misión e historial | Bajo; interno; síncrona | `start_mission` | `create`, `split_segment_mm` | — |
| `RobotService._activate_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L345) | 4 | Python / misión e historial | Medio; interno; síncrona | `start_mission`, `start_return_home` | `_persist_mission`, `_queue_current_mission_step`, `get_setting`, `mission_status`, `publish`, `set_setting` | — |
| `RobotService.start_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L379) | 2 | Python / misión e historial | Bajo; interno; síncrona | `missions` | `_activate_mission`, `_build_segments`, `_require_ready_robot` | telemetría |
| `RobotService.start_return_home` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L387) | 8 | Python / misión e historial | Bajo; interno; síncrona | `return_home` | `_activate_mission`, `_require_ready_robot`, `get_setting`, `set_setting` | telemetría |
| `RobotService.stop_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L426) | 7 | Python / misión e historial | Medio; interno; síncrona | `clear_robot_mission_memory`, `missions` | `_persist_mission`, `_set_return_state`, `cancel`, `get_setting`, `mission_status`, `publish`, `send_command`, `set_setting`, `update_command` | — |
| `RobotService.clear_robot_mission_memory` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L472) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `stop_mission` | — |
| `RobotService._advance_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L478) | 10 | Python / misión e historial | Medio; interno; síncrona | `_on_robot_message`, `_reconcile_short_memory_hello` | `_persist_mission`, `_queue_current_mission_step`, `_queue_final_alignment`, `_set_return_state`, `mission_status`, `publish`, `set_setting` | — |
| `RobotService._set_return_state` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L520) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_advance_mission`, `_archive_mission_for_close`, `_block_mission`, `stop_mission` | `get_setting`, `set_setting` | — |
| `RobotService._queue_current_mission_step` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L527) | 5 | Python / misión e historial | Bajo; interno; síncrona | `_activate_mission`, `_advance_mission`, `_reconcile_short_memory_hello` | `_persist_mission`, `send_command` | — |
| `RobotService._queue_final_alignment` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L554) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_advance_mission`, `_reconcile_short_memory_hello` | `_persist_mission`, `send_command` | — |
| `RobotService._expire_stalled_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L568) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_on_robot_message` | `_block_mission` | — |
| `RobotService._block_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L579) | 6 | Python / misión e historial | Medio; interno; síncrona | `_expire_stalled_mission`, `_on_robot_message`, `_reconcile_mission_snapshot`, `_reconcile_short_memory_hello` | `_persist_mission`, `_set_return_state`, `cancel`, `mission_status`, `publish`, `send_command`, `update_command` | — |
| `RobotService._reconcile_mission_snapshot` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L607) | 8 | Python / misión e historial | Medio; sin llamada interna detectada; síncrona | — | `_block_mission`, `_reconcile_firmware_mission` | — |
| `RobotService._reconcile_firmware_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L633) | 9 | Python / misión e historial | Medio; interno; síncrona | `_reconcile_mission_snapshot` | `_persist_mission`, `mission_status`, `publish` | — |
| `RobotService.send_command` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L667) | 13 | Python / misión e historial | Medio; interno; síncrona | `_block_mission`, `_queue_current_mission_step`, `_queue_final_alignment`, `_reconcile_short_memory_hello`, `command`, `estop`, `prepare_close`, `stop_mission` | `add`, `create`, `enqueue`, `insert_command`, `publish`, `set_setting`, `update_command` | parada/cierre |
| `RobotService.start_session` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L703) | 2 | Python / misión e historial | Medio; entrada/framework; síncrona | `_on_connection_state` | `create_session`, `publish` | sesión |
| `RobotService.stop_session` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L715) | 2 | Python / misión e historial | Medio; entrada/framework; síncrona | `_finish_close_preparation`, `_on_connection_state`, `close` | `publish` | sesión |
| `RobotService._on_connection_state` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L724) | 4 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `insert_event`, `publish`, `start_session`, `stop_session` | — |
| `RobotService._on_command_sent` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L735) | 4 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `publish`, `update_command` | — |
| `RobotService._on_robot_message` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L744) | 19 | Python / misión e historial | Medio; sin llamada interna detectada; síncrona | — | `_advance_mission`, `_block_mission`, `_expire_stalled_mission`, `_reconcile_short_memory_hello`, `from_message`, `insert_event`, `public_dict`, `publish`, `snapshot`, `submit`, `telemetry_priority_signature`, `update_command`, `update_session_identity` | telemetría, parada/cierre |
| `RobotService._reconcile_short_memory_hello` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L834) | 9 | Python / misión e historial | Medio; interno; síncrona | `_on_robot_message` | `_advance_mission`, `_block_mission`, `_queue_current_mission_step`, `_queue_final_alignment`, `insert_event`, `send_command`, `update_command` | — |
| `RobotService._movement_requires_stop` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L876) | 3 | Python / misión e historial | Bajo; interno; síncrona | `prepare_close` | — | telemetría |
| `RobotService._archive_mission_for_close` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L887) | 3 | Python / misión e historial | Medio; interno; síncrona | `_finish_close_preparation` | `_persist_mission`, `_set_return_state`, `cancel`, `mission_status`, `publish` | — |
| `RobotService._finish_close_preparation` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L906) | 6 | Python / misión e historial | Medio; interno; síncrona | `prepare_close` | `_archive_mission_for_close`, `fail_nonterminal_commands`, `insert_event`, `mission_status`, `stop_session` | — |
| `RobotService.prepare_close` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L935) | 11 | Python / misión e historial | Medio; interno; síncrona | `close_application` | `_finish_close_preparation`, `_movement_requires_stop`, `insert_event`, `send_command`, `snapshot` | — |
| `RobotService.purge_sessions` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L990) | 1 | Python / misión e historial | Bajo; interno; síncrona | `cleanup_sessions` | — | — |
| `_service` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L24) | 1 | Python / misión e historial | Bajo; interno; síncrona | `cleanup_sessions`, `clear_robot_mission_memory`, `close_application`, `command`, `connection`, `estop`, `events`, `export_session_json`, `export_telemetry`, `missions`, `return_home`, `robot_config`, `sessions`, `start_session`, `status`, `stop_session` | — | — |
| `_require_token` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L28) | 2 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `wrapper` | — |
| `_require_token.wrapper` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L30) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_require_token` | — | — |
| `index` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L39) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | — | — |
| `ui_asset` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L49) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | — | — |
| `status` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L57) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | `connection`, `events`, `stream` | `_service` | — |
| `robot_config` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L63) | 3 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service`, `get_robot_host`, `set_robot_host` | — |
| `connection` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L77) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service`, `status` | — |
| `command` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L89) | 3 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service`, `send_command` | — |
| `estop` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L102) | 1 | Python / misión e historial | Medio; entrada/framework; síncrona | — | `_service`, `send_command` | parada/cierre |
| `missions` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L109) | 5 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service`, `mission_status`, `start_mission`, `stop_mission` | — |
| `clear_robot_mission_memory` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L126) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service` | — |
| `return_home` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L132) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service`, `start_return_home` | — |
| `close_application` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L141) | 3 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service`, `prepare_close`, `start` | parada/cierre |
| `start_session` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L154) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | `_on_connection_state` | `_service` | — |
| `stop_session` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L160) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | `_finish_close_preparation`, `_on_connection_state`, `close` | `_service` | — |
| `cleanup_sessions` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L166) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `_service`, `purge_sessions` | — |
| `export_telemetry` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L178) | 2 | Python / misión e historial | Medio; entrada/framework; síncrona | — | `_service`, `telemetry_rows` | telemetría, sesión |
| `sessions` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L195) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | `create_session` | `_service`, `session_rows` | — |
| `export_session_json` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L200) | 5 | Python / misión e historial | Medio; entrada/framework; síncrona | — | `_service`, `command_rows`, `event_rows`, `session_row`, `telemetry_rows` | telemetría, sesión |
| `events` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L218) | 3 | Python / misión e historial | Bajo; entrada/framework; cola/evento | `insert_event` | `_service`, `status`, `stream`, `subscribe`, `unsubscribe` | — |
| `events.stream` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L223) | 3 | Python / misión e historial | Bajo; interno; cola/evento | `events` | `status`, `unsubscribe` | — |
