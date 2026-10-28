/**
 * Ambient global declarations for HMI modular scripts.
 */

declare var Chart: any;
declare var ChartDataLabels: any;

interface Waypoint {
  x: number;
  y: number;
  r?: number;
}

interface LogicalStep {
  type: "drive" | "turn" | "step" | "turn_to" | string;
  distance?: number;
  angle?: number;
  speed?: number;
  x?: number;
  y?: number;
  raw?: any;
}

interface Window {
  RobotHost?: {
    closeApp?: () => void;
  };
}

interface RobotJoystick {
  init(): void;
  start(e: PointerEvent): void;
  move(e: PointerEvent): void;
  end(e?: PointerEvent): void;
  setCapability(enabled: boolean): void;
  hasCapability(): boolean;
}

interface HmiState {
  connected: boolean;
  demoMode: boolean;
  estado: string;
  estadoAnterior: string;
  telemetria: {
    distancia: number;
    angulo: number;
    posX: number;
    posY: number;
    orientacion: number;
    pfl: number;
    pfr: number;
    pbl: number;
    pbr: number;
    pwmL: number;
    pwmR: number;
  };
  clientes: number;
  trail: Array<{ x: number; y: number }>;
  logFilters: { info: boolean; warn: boolean; error: boolean; cmd: boolean };
  motorDir: { left: string; right: string };
  activeTab: string;
  joy: {
    active: boolean;
    nx: number;
    ny: number;
    magnitude: number;
    angle: number;
    pwmL: number;
    pwmR: number;
    dirL: string;
    dirR: string;
  };
  manualDriveSupported: boolean;
}

declare var robotChart: any;
declare var totalDistance: number;
declare var totalPoints: number;
declare var currentPoint: number;
declare var lastPoint: { x: number; y: number };
declare function _createScatterChart(ctx: any, chartTheme: CSSStyleDeclaration): any;
declare function initChart(): void;
declare function actualizarBarra(): void;
declare function normalizeSignedDegrees(value: number | string): number;
declare var AngularRouteMode: any;

declare var CANONICAL_CONFIG: {
  readonly PROTOCOL_NAME: string;
  readonly EXPECTED_FIRMWARE: string;
  readonly SUPPORTED_COMMANDS: ReadonlySet<string>;
};

declare var CONFIG: {
  readonly MAX_RPM_VISUAL: number;
  readonly MIN_RPM_VISUAL: number;
  readonly TRAIL_MAX: number;
  readonly LOG_MAX: number;
  readonly JOYSTICK_MAX_RADIUS: number;
  readonly JOYSTICK_DEAD_ZONE: number;
};

declare var state: HmiState;

declare var WSControl: any;
declare var SurfaceManager: any;
declare var Calibration: any;
declare var Telemetry: any;
declare var MotorViz: any;
declare var EncoderSpeed: any;
declare var FSMDisplay: any;
declare var BoostHUD: any;
declare var ForkRegistry: any;
declare var HMI_FORK_REGISTRY: any;
declare var WindowInteract: any;
declare var WindowLayout: any;
declare var WindowManager: any;
declare var RouteModes: any;
declare var ROUTE_PLANNING_MODES: any;
declare var VectorNode: any;
declare var VectorRouteStack: any;
declare var RouteVectorStack: any;
declare var RouteForks: any;
declare var RouteForksManager: any;
declare var RouteCharts: any;
declare var RouteOckham: any;
declare var RouteOckhamCockpit: any;
declare var RouteManager: any;
declare var TabManager: any;
declare var SettingsManager: any;
declare var ConnectionPanel: any;
declare var LogSystem: any;
declare var I18N: Record<string, Record<string, string>>;
