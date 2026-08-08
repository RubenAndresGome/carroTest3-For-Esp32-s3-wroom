# UML funcional: `backend/python`

Funciones detectadas: **118**. Tipos detectados: **12**.

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
  subgraph f3["config.py"]
    n2c28dde60e["normalize_robot_host(value: object)"]
    n9e2127431d["from_environment(cls)"]
  end
  subgraph f4["database.py"]
    n86f7e28f5c["__init__(self, path: Path)"]
    n05766f4679["connect(self)"]
    n23fb7c8d16["transaction(self)"]
    ne45be1344d["initialize(self)"]
    n9666ef0659["get_setting(self, key: str, default: Any = None)"]
    n69d89b1d8c["set_setting(self, key: str, value: Any)"]
    n3898e6f467["create_session(self)"]
    ne90d71830a["update_session_identity(self, session_id: int, robot_id: str | None, firmware_version: str | None, protocol: str | None)"]
    n83d5115c83["stop_session(self, session_id: int, reason: str | None = None)"]
    n27ae570fe9["insert_command(self, command_id: str, session_id: int | None, name: str, payload: dict[str, Any], status: str)"]
    n76e8386fea["update_command(self, command_id: str, status: str, error: str | None = None)"]
    n46dc5cc68e["fail_nonterminal_commands(self, reason: str)"]
    ncead72cfeb["close_orphan_sessions(self, reason: str)"]
    n9200c1431e["insert_event(self, session_id: int | None, kind: str, severity: str, payload: dict[str, Any])"]
    nceb90c202f["insert_telemetry(self, session_id: int, snapshot: Any)"]
    na62e9c42fc["telemetry_rows(self, session_id: int)"]
    nb756e00a7f["session_rows(self)"]
    nb561ffbc05["session_row(self, session_id: int)"]
    n719abc7e65["command_rows(self, session_id: int)"]
    n091ba89cf1["event_rows(self, session_id: int)"]
    n16e08bed59["purge_sessions(self, days: int)"]
  end
  subgraph f5["domain.py"]
    nb983649585["_finite_number(value: Any, name: str, low: float, high: float)"]
    n5dae047b8f["_heading_degrees(value: Any)"]
    n8e2fbfdb5d["validate_command_payload(name: str, payload: Mapping[str, Any] | None)"]
    n67c3bdbb31["create(cls, name: object, payload: Mapping[str, Any] | None = None, seq: int = 0, command_id: str | None = None)"]
    nede5d2494e["protocol_envelope(self)"]
    n918003cd14["split_segment_mm( start_x_mm: float, start_y_mm: float, end_x_mm: float, end_y_mm: float, max_segment_mm: float = MAX_SEGMENT_MM, )"]
    na677870675["from_message(cls, message: Mapping[str, Any], fallback_sequence: int)"]
    ndf8487517c["public_dict(self)"]
  end
  subgraph f6["gateway.py"]
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
  subgraph f7["services.py"]
    nc58992ce45["__init__(self, subscriber_size: int = 64)"]
    na9729b1c67["subscribe(self)"]
    n88563040de["unsubscribe(self, subscriber: queue.Queue[str])"]
    n4c576330b1["publish(self, kind: str, payload: dict[str, Any])"]
    n34e81b855a["__init__(self, database: Database, queue_size: int = 256)"]
    n55583f40a2["submit(self, session_id: int, snapshot: TelemetrySnapshot)"]
    nf78d3ee8b1["stop(self)"]
    n76f6a478db["_run(self)"]
    n896832408a["__init__(self, database: Database, start_gateway: bool = True, max_message_bytes: int = 4096)"]
    n332b622517["close(self)"]
    ne14be5f2cb["get_robot_host(self)"]
    ndc211d5b95["set_robot_host(self, value: object)"]
    n248d2dce7d["connect(self)"]
    nc52dd740aa["disconnect(self)"]
    n129ecd6bf8["reconnect(self)"]
    n7d5ebbc8b4["status(self)"]
    n3b759c213f["mission_status(self)"]
    n9a0f6ac366["_persist_mission(self)"]
    nf24d5e6c21["_mission_wire_points(self)"]
    ndddad585fb["_require_ready_robot(self)"]
    n230292e845["_build_segments(self, points: list[Any], mission_origin: dict[str, float])"]
    n5904f4d532["_activate_mission(self, segments: list[dict[str, float]], mission_origin: dict[str, float], kind: str, final_heading: float | None = None)"]
    n35e10ad2f6["start_mission(self, points: object)"]
    n1d52dd4c0f["start_return_home(self)"]
    n19d904450d["stop_mission(self, reason: str = 'operator_stop')"]
    nd8734ad52f["clear_robot_mission_memory(self)"]
    n1a9e2ce29f["_advance_mission(self)"]
    n5b7c497ab8["_set_return_state(self, state: str, error: str | None = None)"]
    nf0737ba032["_queue_current_mission_step(self, seq_override: int | None = None)"]
    n244f8444b7["_queue_final_alignment(self, seq_override: int | None = None)"]
    nf5cff9c6eb["_expire_stalled_mission(self)"]
    n4979c10981["_block_mission(self, reason: str)"]
    nef2199af78["_reconcile_mission_snapshot(self, snapshot: TelemetrySnapshot)"]
    n072122a3f8["_reconcile_firmware_mission(self, firmware: dict[str, Any])"]
    nc6fa0f1a20["send_command(self, name: object, payload: dict[str, Any] | None, seq_override: int | None = None, command_id_override: str | None = None)"]
    n96e7060441["start_session(self)"]
    nf0c0061b26["stop_session(self, reason: str | None = None)"]
    nbd9caf92eb["_on_connection_state(self, state: ConnectionState, detail: str | None)"]
    n3aea915da2["_on_command_sent(self, command: RobotCommand)"]
    n57ad37b856["_on_robot_message(self, message: dict[str, Any])"]
    n3bc9ebfff8["_reconcile_short_memory_hello(self, message: dict[str, Any])"]
    n37e61be468["_movement_requires_stop(self)"]
    n1bca975513["_archive_mission_for_close(self)"]
    n5f7b2f0f67["_finish_close_preparation(self, force: bool, stop_required: bool, stop_confirmed: bool)"]
    ne446ac80b8["prepare_close(self, force: bool = False, timeout_s: float | None = None)"]
    nea26546a12["purge_sessions(self, days: int)"]
  end
  subgraph f8["web.py"]
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
    nefd77277fd["sessions()"]
    n2ce00fdeb1["export_session_json(session_id: int)"]
    n40caa891fe["events()"]
    n2a6d149e68["stream()"]
  end
  n06e0517455 --> n75cdaae440
  n072122a3f8 --> n3b759c213f
  n072122a3f8 --> n4c576330b1
  n072122a3f8 --> n9a0f6ac366
  n091ba89cf1 --> n05766f4679
  n129ecd6bf8 --> n7e9b9be06c
  n129ecd6bf8 --> n9c7f87a274
  n129ecd6bf8 --> nc197fe04ab
  n13eb31c6b3 --> n11a7fd3b87
  n13eb31c6b3 --> na62e9c42fc
  n1520d57fd8 --> n332b622517
  n1520d57fd8 --> n4aa436b2b5
  n1520d57fd8 --> n5a1cf90c3e
  n1520d57fd8 --> n75cdaae440
  n16e08bed59 --> n23fb7c8d16
  n18b113cef4 --> n11a7fd3b87
  n18b113cef4 --> n9c7f87a274
  n18b113cef4 --> ne446ac80b8
  n19d904450d --> n3b759c213f
  n19d904450d --> n4c576330b1
  n19d904450d --> n5b7c497ab8
  n19d904450d --> n69d89b1d8c
  n19d904450d --> n76e8386fea
  n19d904450d --> n9666ef0659
  n19d904450d --> n9a0f6ac366
  n19d904450d --> nc6fa0f1a20
  n19d904450d --> nd7487f13c1
  n1a9e2ce29f --> n244f8444b7
  n1a9e2ce29f --> n3b759c213f
  n1a9e2ce29f --> n4c576330b1
  n1a9e2ce29f --> n5b7c497ab8
  n1a9e2ce29f --> n69d89b1d8c
  n1a9e2ce29f --> n9a0f6ac366
  n1a9e2ce29f --> nf0737ba032
  n1bca975513 --> n3b759c213f
  n1bca975513 --> n4c576330b1
  n1bca975513 --> n5b7c497ab8
  n1bca975513 --> n9a0f6ac366
  n1bca975513 --> nd7487f13c1
  n1d52dd4c0f --> n5904f4d532
  n1d52dd4c0f --> n69d89b1d8c
  n1d52dd4c0f --> n9666ef0659
  n1d52dd4c0f --> ndddad585fb
  n1eb416b959 --> n11a7fd3b87
  n1eb416b959 --> ndc211d5b95
  n1eb416b959 --> ne14be5f2cb
  n1ef8b6f8a4 --> n11a7fd3b87
  n230292e845 --> n67c3bdbb31
  n230292e845 --> n918003cd14
  n23fb7c8d16 --> n05766f4679
  n23fb7c8d16 --> n332b622517
  n244f8444b7 --> n9a0f6ac366
  n244f8444b7 --> nc6fa0f1a20
  n248d2dce7d --> n9c7f87a274
  n27ae570fe9 --> n23fb7c8d16
  n2a6d149e68 -.-> n7d5ebbc8b4
  n2a6d149e68 -.-> n88563040de
  n2ce00fdeb1 --> n091ba89cf1
  n2ce00fdeb1 --> n11a7fd3b87
  n2ce00fdeb1 --> n719abc7e65
  n2ce00fdeb1 --> na62e9c42fc
  n2ce00fdeb1 --> nb561ffbc05
  n332b622517 --> n06e0517455
  n332b622517 --> n83d5115c83
  n34e81b855a -.-> n9c7f87a274
  n35e10ad2f6 --> n230292e845
  n35e10ad2f6 --> n5904f4d532
  n35e10ad2f6 --> ndddad585fb
  n3898e6f467 --> n23fb7c8d16
  n3898e6f467 --> nefd77277fd
  n3aea915da2 --> n4c576330b1
  n3aea915da2 --> n76e8386fea
  n3bc9ebfff8 --> n1a9e2ce29f
  n3bc9ebfff8 --> n244f8444b7
  n3bc9ebfff8 --> n4979c10981
  n3bc9ebfff8 --> n76e8386fea
  n3bc9ebfff8 --> n9200c1431e
  n3bc9ebfff8 --> nc6fa0f1a20
  n3bc9ebfff8 --> nf0737ba032
  n3e89fea1ba --> n332b622517
  n40caa891fe -.-> n11a7fd3b87
  n40caa891fe -.-> n2a6d149e68
  n40caa891fe -.-> n7d5ebbc8b4
  n40caa891fe -.-> n88563040de
  n40caa891fe -.-> na9729b1c67
  n46dc5cc68e --> n23fb7c8d16
  n4979c10981 --> n3b759c213f
  n4979c10981 --> n4c576330b1
  n4979c10981 --> n5b7c497ab8
  n4979c10981 --> n76e8386fea
  n4979c10981 --> n9a0f6ac366
  n4979c10981 --> nc6fa0f1a20
  n4979c10981 --> nd7487f13c1
  n57ad37b856 --> n1a9e2ce29f
  n57ad37b856 --> n3bc9ebfff8
  n57ad37b856 --> n4979c10981
  n57ad37b856 --> n4c576330b1
  n57ad37b856 --> n55583f40a2
  n57ad37b856 --> n76e8386fea
  n57ad37b856 --> n9200c1431e
  n57ad37b856 --> na677870675
  n57ad37b856 --> nc197fe04ab
  n57ad37b856 --> ndf8487517c
  n57ad37b856 --> ne90d71830a
  n57ad37b856 --> nf5cff9c6eb
  n5904f4d532 --> n3b759c213f
  n5904f4d532 --> n4c576330b1
  n5904f4d532 --> n69d89b1d8c
  n5904f4d532 --> n9666ef0659
  n5904f4d532 --> n9a0f6ac366
  n5904f4d532 --> nf0737ba032
  n5a1cf90c3e -.-> nede5d2494e
  n5b7c497ab8 --> n69d89b1d8c
  n5b7c497ab8 --> n9666ef0659
  n5dae047b8f --> nb983649585
  n5f28d62235 --> n11a7fd3b87
  n5f7b2f0f67 --> n1bca975513
  n5f7b2f0f67 --> n3b759c213f
  n5f7b2f0f67 --> n46dc5cc68e
  n5f7b2f0f67 --> n83d5115c83
  n5f7b2f0f67 --> n9200c1431e
  n67c3bdbb31 --> n8e2fbfdb5d
  n69d89b1d8c --> n23fb7c8d16
  n69f79285a5 --> n11a7fd3b87
  n719abc7e65 --> n05766f4679
  n73c8d4811c --> n11a7fd3b87
  n73c8d4811c --> n7d5ebbc8b4
  n76e8386fea --> n23fb7c8d16
  n76f6a478db -.-> nceb90c202f
  n7d5ebbc8b4 --> n3b759c213f
  n7d5ebbc8b4 --> nc197fe04ab
  n7d5ebbc8b4 --> ndf8487517c
  n815823909d --> n11a7fd3b87
  n83d5115c83 --> n23fb7c8d16
  n896832408a --> n2c28dde60e
  n896832408a --> n46dc5cc68e
  n896832408a --> n69d89b1d8c
  n896832408a --> n9200c1431e
  n896832408a --> n9666ef0659
  n896832408a --> n9a0f6ac366
  n896832408a --> n9c7f87a274
  n896832408a --> ncead72cfeb
  n8e2fbfdb5d --> n5dae047b8f
  n8e2fbfdb5d --> nb983649585
  n8fdba82311 --> n11a7fd3b87
  n8fdba82311 --> n19d904450d
  n8fdba82311 --> n35e10ad2f6
  n8fdba82311 --> n3b759c213f
  n918003cd14 --> nb983649585
  n9200c1431e --> n23fb7c8d16
  n9200c1431e --> n40caa891fe
  n9222a02d25 --> n332b622517
  n9222a02d25 --> n3e89fea1ba
  n9222a02d25 --> n4303201550
  n9222a02d25 --> n9c7f87a274
  n9222a02d25 --> nd5db088795
  n9222a02d25 --> nfb667eae25
  n93a37929af --> n11a7fd3b87
  n93a37929af --> nc6fa0f1a20
  n9666ef0659 --> n05766f4679
  n96e7060441 --> n3898e6f467
  n96e7060441 --> n4c576330b1
  n9768753665 --> nab39be2bb6
  n9a0f6ac366 --> n69d89b1d8c
  n9c7f87a274 --> nc197fe04ab
  n9e2127431d --> n2c28dde60e
  na13c11dcdf --> n11a7fd3b87
  na13c11dcdf --> n1d52dd4c0f
  na5e8d26bb3 --> n11a7fd3b87
  na5e8d26bb3 --> n16e08bed59
  na62e9c42fc --> n05766f4679
  na677870675 --> n5dae047b8f
  na677870675 --> nb983649585
  nb561ffbc05 --> n05766f4679
  nb756e00a7f --> n05766f4679
  nbd9caf92eb --> n4c576330b1
  nbd9caf92eb --> n83d5115c83
  nbd9caf92eb --> n9200c1431e
  nbd9caf92eb --> n96e7060441
  nc52dd740aa --> n06e0517455
  nc6fa0f1a20 --> n27ae570fe9
  nc6fa0f1a20 --> n4c576330b1
  nc6fa0f1a20 --> n67c3bdbb31
  nc6fa0f1a20 --> n69d89b1d8c
  nc6fa0f1a20 --> n76e8386fea
  nc6fa0f1a20 --> ne1b1b4ed58
  ncead72cfeb --> n23fb7c8d16
  nceb90c202f --> n23fb7c8d16
  nd45a072485 --> n11a7fd3b87
  nd45a072485 --> nc6fa0f1a20
  nd8734ad52f --> n19d904450d
  ndc211d5b95 --> n2c28dde60e
  ndc211d5b95 --> n4c576330b1
  ndc211d5b95 --> n69d89b1d8c
  ndc211d5b95 --> n7e9b9be06c
  ndddad585fb --> nc197fe04ab
  ne446ac80b8 --> n37e61be468
  ne446ac80b8 --> n5f7b2f0f67
  ne446ac80b8 --> n9200c1431e
  ne446ac80b8 --> nc197fe04ab
  ne446ac80b8 --> nc6fa0f1a20
  ne45be1344d --> n05766f4679
  ne45be1344d --> n332b622517
  ne90d71830a --> n23fb7c8d16
  nef2199af78 --> n072122a3f8
  nef2199af78 --> n4979c10981
  nefd77277fd --> n11a7fd3b87
  nefd77277fd --> nb756e00a7f
  nf0737ba032 --> n9a0f6ac366
  nf0737ba032 --> nc6fa0f1a20
  nf0c0061b26 --> n4c576330b1
  nf5cff9c6eb --> n4979c10981
  nfb676358e5 --> n69d89b1d8c
  nfb676358e5 --> n7502f63d12
  nfb676358e5 --> n8019a4b26e
  nfb676358e5 --> n9666ef0659
  nfb676358e5 --> n9e2127431d
  nfb676358e5 --> ne45be1344d
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
  class n2c28dde60e bajo
  class n9e2127431d medio
  class n86f7e28f5c bajo
  class n05766f4679 bajo
  class n23fb7c8d16 bajo
  class ne45be1344d bajo
  class n9666ef0659 bajo
  class n69d89b1d8c bajo
  class n3898e6f467 bajo
  class ne90d71830a bajo
  class n83d5115c83 bajo
  class n27ae570fe9 bajo
  class n76e8386fea bajo
  class n46dc5cc68e bajo
  class ncead72cfeb bajo
  class n9200c1431e bajo
  class nceb90c202f bajo
  class na62e9c42fc bajo
  class nb756e00a7f bajo
  class nb561ffbc05 bajo
  class n719abc7e65 bajo
  class n091ba89cf1 bajo
  class n16e08bed59 bajo
  class nb983649585 bajo
  class n5dae047b8f bajo
  class n8e2fbfdb5d bajo
  class n67c3bdbb31 bajo
  class nede5d2494e bajo
  class n918003cd14 bajo
  class na677870675 medio
  class ndf8487517c medio
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
  class n896832408a bajo
  class n332b622517 medio
  class ne14be5f2cb bajo
  class ndc211d5b95 bajo
  class n248d2dce7d bajo
  class nc52dd740aa medio
  class n129ecd6bf8 bajo
  class n7d5ebbc8b4 medio
  class n3b759c213f bajo
  class n9a0f6ac366 bajo
  class nf24d5e6c21 bajo
  class ndddad585fb bajo
  class n230292e845 bajo
  class n5904f4d532 medio
  class n35e10ad2f6 bajo
  class n1d52dd4c0f bajo
  class n19d904450d medio
  class nd8734ad52f bajo
  class n1a9e2ce29f medio
  class n5b7c497ab8 bajo
  class nf0737ba032 bajo
  class n244f8444b7 bajo
  class nf5cff9c6eb bajo
  class n4979c10981 medio
  class nef2199af78 medio
  class n072122a3f8 medio
  class nc6fa0f1a20 medio
  class n96e7060441 medio
  class nf0c0061b26 medio
  class nbd9caf92eb bajo
  class n3aea915da2 bajo
  class n57ad37b856 bajo
  class n3bc9ebfff8 medio
  class n37e61be468 bajo
  class n1bca975513 medio
  class n5f7b2f0f67 medio
  class ne446ac80b8 medio
  class nea26546a12 bajo
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
  class nefd77277fd bajo
  class n2ce00fdeb1 medio
  class n40caa891fe bajo
  class n2a6d149e68 bajo
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
| `normalize_robot_host` | [`desktop_app/robot_app/config.py`](../../desktop_app/robot_app/config.py#L16) | 6 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `from_environment`, `set_robot_host` | — | — |
| `AppConfig.from_environment` | [`desktop_app/robot_app/config.py`](../../desktop_app/robot_app/config.py#L39) | 1 | Python / misión e historial | Medio; interno; asíncrona | `create_app` | `normalize_robot_host` | — |
| `Database.__init__` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L13) | 1 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `Database.connect` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L17) | 1 | Python / misión e historial | Bajo; interno; síncrona | `command_rows`, `connect_db`, `event_rows`, `get_setting`, `initialize`, `main`, `session_row`, `session_rows`, `telemetry_rows`, `transaction` | — | — |
| `Database.transaction` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L27) | 3 | Python / misión e historial | Bajo; interno; síncrona | `close_orphan_sessions`, `create_session`, `fail_nonterminal_commands`, `insert_command`, `insert_event`, `insert_telemetry`, `purge_sessions`, `set_setting`, `stop_session`, `update_command`, `update_session_identity` | `close`, `connect` | — |
| `Database.initialize` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L40) | 5 | Python / misión e historial | Bajo; interno; síncrona | `create_app` | `close`, `connect` | telemetría |
| `Database.get_setting` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L65) | 3 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `_activate_mission`, `_set_return_state`, `create_app`, `start_return_home`, `stop_mission` | `connect` | parada/cierre |
| `Database.set_setting` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L70) | 3 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `_activate_mission`, `_advance_mission`, `_persist_mission`, `_set_return_state`, `create_app`, `send_command`, `set_robot_host`, `start_return_home`, `stop_mission` | `transaction` | — |
| `Database.create_session` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L79) | 1 | Python / misión e historial | Bajo; interno; síncrona | `start_session` | `sessions`, `transaction` | — |
| `Database.update_session_identity` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L84) | 5 | Python / misión e historial | Bajo; interno; síncrona | `_on_robot_message` | `transaction` | — |
| `Database.stop_session` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L91) | 3 | Python / misión e historial | Bajo; entrada/framework; síncrona | `_finish_close_preparation`, `_on_connection_state`, `close` | `transaction` | — |
| `Database.insert_command` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L98) | 6 | Python / misión e historial | Bajo; interno; síncrona | `send_command` | `transaction` | — |
| `Database.update_command` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L105) | 6 | Python / misión e historial | Bajo; interno; síncrona | `_block_mission`, `_on_command_sent`, `_on_robot_message`, `_reconcile_short_memory_hello`, `send_command`, `stop_mission` | `transaction` | — |
| `Database.fail_nonterminal_commands` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L117) | 2 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `_finish_close_preparation` | `transaction` | — |
| `Database.close_orphan_sessions` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L127) | 2 | Python / misión e historial | Bajo; interno; síncrona | `__init__` | `transaction` | — |
| `Database.insert_event` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L135) | 5 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `_finish_close_preparation`, `_on_connection_state`, `_on_robot_message`, `_reconcile_short_memory_hello`, `prepare_close` | `events`, `transaction` | — |
| `Database.insert_telemetry` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L142) | 20 | Python / misión e historial | Bajo; interno; síncrona | `_run` | `transaction` | telemetría |
| `Database.telemetry_rows` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L154) | 2 | Python / misión e historial | Bajo; interno; síncrona | `export_session_json`, `export_telemetry` | `connect` | telemetría, parada/cierre |
| `Database.session_rows` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L161) | 1 | Python / misión e historial | Bajo; interno; síncrona | `sessions` | `connect` | telemetría, parada/cierre |
| `Database.session_row` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L171) | 2 | Python / misión e historial | Bajo; interno; síncrona | `export_session_json` | `connect` | parada/cierre |
| `Database.command_rows` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L175) | 2 | Python / misión e historial | Bajo; interno; síncrona | `export_session_json` | `connect` | parada/cierre |
| `Database.event_rows` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L182) | 2 | Python / misión e historial | Bajo; interno; síncrona | `export_session_json` | `connect` | parada/cierre |
| `Database.purge_sessions` | [`desktop_app/robot_app/database.py`](../../desktop_app/robot_app/database.py#L189) | 9 | Python / misión e historial | Bajo; interno; síncrona | `cleanup_sessions` | `transaction` | telemetría |
| `_finite_number` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L45) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_heading_degrees`, `from_message`, `split_segment_mm`, `validate_command_payload` | — | — |
| `_heading_degrees` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L54) | 1 | Python / misión e historial | Bajo; interno; síncrona | `from_message`, `validate_command_payload` | `_finite_number` | — |
| `validate_command_payload` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L58) | 10 | Python / misión e historial | Bajo; interno; síncrona | `create` | `_finite_number`, `_heading_degrees` | — |
| `RobotCommand.create` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L101) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_build_segments`, `send_command` | `validate_command_payload` | — |
| `RobotCommand.protocol_envelope` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L115) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_drain_one` | — | — |
| `split_segment_mm` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L121) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_build_segments` | `_finite_number` | — |
| `TelemetrySnapshot.from_message` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L242) | 37 | Python / misión e historial | Medio; interno; síncrona | `_on_robot_message` | `_finite_number`, `_heading_degrees` | telemetría |
| `TelemetrySnapshot.public_dict` | [`desktop_app/robot_app/domain.py`](../../desktop_app/robot_app/domain.py#L411) | 1 | Python / misión e historial | Medio; interno; síncrona | `_on_robot_message`, `status` | — | — |
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
| `RobotService.__init__` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L92) | 11 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `_persist_mission`, `close_orphan_sessions`, `fail_nonterminal_commands`, `get_setting`, `insert_event`, `normalize_robot_host`, `set_setting`, `start` | misión activa |
| `RobotService.close` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L187) | 3 | Python / misión e historial | Medio; interno; síncrona | `_run`, `closeApplication`, `initialize`, `main`, `run`, `shutdown_host`, `stop`, `transaction` | `stop`, `stop_session` | — |
| `RobotService.get_robot_host` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L199) | 1 | Python / misión e historial | Bajo; interno; síncrona | `robot_config` | — | — |
| `RobotService.set_robot_host` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L203) | 2 | Python / misión e historial | Bajo; interno; síncrona | `robot_config` | `normalize_robot_host`, `publish`, `request_reconnect`, `set_setting` | — |
| `RobotService.connect` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L214) | 1 | Python / misión e historial | Bajo; interno; síncrona | `command_rows`, `connect_db`, `event_rows`, `get_setting`, `initialize`, `main`, `session_row`, `session_rows`, `telemetry_rows`, `transaction` | `start` | — |
| `RobotService.disconnect` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L217) | 1 | Python / misión e historial | Medio; sin llamada interna detectada; síncrona | — | `stop` | — |
| `RobotService.reconnect` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L220) | 2 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `request_reconnect`, `snapshot`, `start` | — |
| `RobotService.status` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L226) | 2 | Python / misión e historial | Medio; entrada/framework; síncrona | `connection`, `events`, `stream` | `mission_status`, `public_dict`, `snapshot` | telemetría, parada/cierre |
| `RobotService.mission_status` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L244) | 4 | Python / misión e historial | Bajo; interno; síncrona | `_activate_mission`, `_advance_mission`, `_archive_mission_for_close`, `_block_mission`, `_finish_close_preparation`, `_reconcile_firmware_mission`, `missions`, `status`, `stop_mission` | — | — |
| `RobotService._persist_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L266) | 2 | Python / misión e historial | Bajo; interno; síncrona | `__init__`, `_activate_mission`, `_advance_mission`, `_archive_mission_for_close`, `_block_mission`, `_queue_current_mission_step`, `_queue_final_alignment`, `_reconcile_firmware_mission`, `stop_mission` | `set_setting` | misión activa |
| `RobotService._mission_wire_points` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L279) | 2 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | — | — |
| `RobotService._require_ready_robot` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L288) | 7 | Python / misión e historial | Bajo; interno; síncrona | `start_mission`, `start_return_home` | `snapshot` | estado, telemetría |
| `RobotService._build_segments` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L306) | 9 | Python / misión e historial | Bajo; interno; síncrona | `start_mission` | `create`, `split_segment_mm` | — |
| `RobotService._activate_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L341) | 4 | Python / misión e historial | Medio; interno; síncrona | `start_mission`, `start_return_home` | `_persist_mission`, `_queue_current_mission_step`, `get_setting`, `mission_status`, `publish`, `set_setting` | — |
| `RobotService.start_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L375) | 2 | Python / misión e historial | Bajo; interno; síncrona | `missions` | `_activate_mission`, `_build_segments`, `_require_ready_robot` | telemetría |
| `RobotService.start_return_home` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L383) | 8 | Python / misión e historial | Bajo; interno; síncrona | `return_home` | `_activate_mission`, `_require_ready_robot`, `get_setting`, `set_setting` | telemetría |
| `RobotService.stop_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L422) | 7 | Python / misión e historial | Medio; interno; síncrona | `clear_robot_mission_memory`, `missions` | `_persist_mission`, `_set_return_state`, `cancel`, `get_setting`, `mission_status`, `publish`, `send_command`, `set_setting`, `update_command` | — |
| `RobotService.clear_robot_mission_memory` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L468) | 1 | Python / misión e historial | Bajo; entrada/framework; síncrona | — | `stop_mission` | — |
| `RobotService._advance_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L474) | 10 | Python / misión e historial | Medio; interno; síncrona | `_on_robot_message`, `_reconcile_short_memory_hello` | `_persist_mission`, `_queue_current_mission_step`, `_queue_final_alignment`, `_set_return_state`, `mission_status`, `publish`, `set_setting` | — |
| `RobotService._set_return_state` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L516) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_advance_mission`, `_archive_mission_for_close`, `_block_mission`, `stop_mission` | `get_setting`, `set_setting` | — |
| `RobotService._queue_current_mission_step` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L523) | 5 | Python / misión e historial | Bajo; interno; síncrona | `_activate_mission`, `_advance_mission`, `_reconcile_short_memory_hello` | `_persist_mission`, `send_command` | — |
| `RobotService._queue_final_alignment` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L550) | 3 | Python / misión e historial | Bajo; interno; síncrona | `_advance_mission`, `_reconcile_short_memory_hello` | `_persist_mission`, `send_command` | — |
| `RobotService._expire_stalled_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L564) | 2 | Python / misión e historial | Bajo; interno; síncrona | `_on_robot_message` | `_block_mission` | — |
| `RobotService._block_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L575) | 6 | Python / misión e historial | Medio; interno; síncrona | `_expire_stalled_mission`, `_on_robot_message`, `_reconcile_mission_snapshot`, `_reconcile_short_memory_hello` | `_persist_mission`, `_set_return_state`, `cancel`, `mission_status`, `publish`, `send_command`, `update_command` | — |
| `RobotService._reconcile_mission_snapshot` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L603) | 8 | Python / misión e historial | Medio; sin llamada interna detectada; síncrona | — | `_block_mission`, `_reconcile_firmware_mission` | — |
| `RobotService._reconcile_firmware_mission` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L629) | 9 | Python / misión e historial | Medio; interno; síncrona | `_reconcile_mission_snapshot` | `_persist_mission`, `mission_status`, `publish` | — |
| `RobotService.send_command` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L663) | 13 | Python / misión e historial | Medio; interno; síncrona | `_block_mission`, `_queue_current_mission_step`, `_queue_final_alignment`, `_reconcile_short_memory_hello`, `command`, `estop`, `prepare_close`, `stop_mission` | `add`, `create`, `enqueue`, `insert_command`, `publish`, `set_setting`, `update_command` | parada/cierre |
| `RobotService.start_session` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L699) | 2 | Python / misión e historial | Medio; entrada/framework; síncrona | `_on_connection_state` | `create_session`, `publish` | sesión |
| `RobotService.stop_session` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L708) | 2 | Python / misión e historial | Medio; entrada/framework; síncrona | `_finish_close_preparation`, `_on_connection_state`, `close` | `publish` | sesión |
| `RobotService._on_connection_state` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L717) | 4 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `insert_event`, `publish`, `start_session`, `stop_session` | — |
| `RobotService._on_command_sent` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L728) | 4 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `publish`, `update_command` | — |
| `RobotService._on_robot_message` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L737) | 17 | Python / misión e historial | Bajo; sin llamada interna detectada; síncrona | — | `_advance_mission`, `_block_mission`, `_expire_stalled_mission`, `_reconcile_short_memory_hello`, `from_message`, `insert_event`, `public_dict`, `publish`, `snapshot`, `submit`, `update_command`, `update_session_identity` | telemetría |
| `RobotService._reconcile_short_memory_hello` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L817) | 9 | Python / misión e historial | Medio; interno; síncrona | `_on_robot_message` | `_advance_mission`, `_block_mission`, `_queue_current_mission_step`, `_queue_final_alignment`, `insert_event`, `send_command`, `update_command` | — |
| `RobotService._movement_requires_stop` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L859) | 3 | Python / misión e historial | Bajo; interno; síncrona | `prepare_close` | — | telemetría |
| `RobotService._archive_mission_for_close` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L870) | 3 | Python / misión e historial | Medio; interno; síncrona | `_finish_close_preparation` | `_persist_mission`, `_set_return_state`, `cancel`, `mission_status`, `publish` | — |
| `RobotService._finish_close_preparation` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L889) | 6 | Python / misión e historial | Medio; interno; síncrona | `prepare_close` | `_archive_mission_for_close`, `fail_nonterminal_commands`, `insert_event`, `mission_status`, `stop_session` | — |
| `RobotService.prepare_close` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L918) | 11 | Python / misión e historial | Medio; interno; síncrona | `close_application` | `_finish_close_preparation`, `_movement_requires_stop`, `insert_event`, `send_command`, `snapshot` | — |
| `RobotService.purge_sessions` | [`desktop_app/robot_app/services.py`](../../desktop_app/robot_app/services.py#L973) | 1 | Python / misión e historial | Bajo; interno; síncrona | `cleanup_sessions` | — | — |
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
| `sessions` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L191) | 2 | Python / misión e historial | Bajo; entrada/framework; síncrona | `create_session` | `_service`, `session_rows` | — |
| `export_session_json` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L196) | 5 | Python / misión e historial | Medio; entrada/framework; síncrona | — | `_service`, `command_rows`, `event_rows`, `session_row`, `telemetry_rows` | telemetría, sesión |
| `events` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L214) | 3 | Python / misión e historial | Bajo; entrada/framework; cola/evento | `insert_event` | `_service`, `status`, `stream`, `subscribe`, `unsubscribe` | — |
| `events.stream` | [`desktop_app/robot_app/web.py`](../../desktop_app/robot_app/web.py#L219) | 3 | Python / misión e historial | Bajo; interno; cola/evento | `events` | `status`, `unsubscribe` | — |
