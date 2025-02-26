// TypeScript bindings for emscripten-generated code.  Automatically generated at compile time.
declare namespace RuntimeExports {
    let HEAPF32: any;
    let HEAPF64: any;
    let HEAP_DATA_VIEW: any;
    let HEAP8: any;
    let HEAPU8: any;
    let HEAP16: any;
    let HEAPU16: any;
    let HEAP32: any;
    let HEAPU32: any;
    let HEAP64: any;
    let HEAPU64: any;
}
interface WasmModule {
}

type EmbindString = ArrayBuffer|Uint8Array|Uint8ClampedArray|Int8Array|string;
export interface ClassHandle {
  isAliasOf(other: ClassHandle): boolean;
  delete(): void;
  deleteLater(): this;
  isDeleted(): boolean;
  clone(): this;
}
export interface dxfWriter extends ClassHandle {
  writeUtf8String(_0: number, _1: EmbindString): boolean;
  writeUtf8Caps(_0: number, _1: EmbindString): boolean;
  fromUtf8String(_0: EmbindString): string;
  setVersion(_0: EmbindString, _1: boolean): void;
  setCodePage(_0: EmbindString): void;
  getCodePage(): string;
  writeString(_0: number, _1: EmbindString): boolean;
  writeInt16(_0: number, _1: number): boolean;
  writeInt32(_0: number, _1: number): boolean;
  writeInt64(_0: number, _1: bigint): boolean;
  writeDouble(_0: number, _1: number): boolean;
  writeBool(_0: number, _1: boolean): boolean;
}

export interface DRW_VersionValue<T extends number> {
  value: T;
}
export type DRW_Version = DRW_VersionValue<0>|DRW_VersionValue<1>|DRW_VersionValue<2>|DRW_VersionValue<3>|DRW_VersionValue<4>|DRW_VersionValue<5>|DRW_VersionValue<6>|DRW_VersionValue<7>|DRW_VersionValue<8>|DRW_VersionValue<9>|DRW_VersionValue<10>|DRW_VersionValue<11>|DRW_VersionValue<12>|DRW_VersionValue<13>|DRW_VersionValue<14>|DRW_VersionValue<15>|DRW_VersionValue<16>|DRW_VersionValue<17>|DRW_VersionValue<18>;

export interface DWR_ErrorValue<T extends number> {
  value: T;
}
export type DWR_Error = DWR_ErrorValue<0>|DWR_ErrorValue<1>|DWR_ErrorValue<2>|DWR_ErrorValue<3>|DWR_ErrorValue<4>|DWR_ErrorValue<5>|DWR_ErrorValue<6>|DWR_ErrorValue<7>|DWR_ErrorValue<8>|DWR_ErrorValue<9>|DWR_ErrorValue<10>|DWR_ErrorValue<11>|DWR_ErrorValue<12>|DWR_ErrorValue<13>|DWR_ErrorValue<14>;

export interface DRW_DebugLevelValue<T extends number> {
  value: T;
}
export type DRW_DebugLevel = DRW_DebugLevelValue<0>|DRW_DebugLevelValue<1>;

export interface DebugPrinter extends ClassHandle {
  printS(_0: EmbindString): void;
  printI(_0: bigint): void;
  printUI(_0: bigint): void;
  printD(_0: number): void;
  printH(_0: bigint): void;
  printB(_0: number): void;
  printHL(_0: number, _1: number, _2: number): void;
  printPT(_0: number, _1: number, _2: number): void;
}

export interface DRW_ColorCodesValue<T extends number> {
  value: T;
}
export type DRW_ColorCodes = DRW_ColorCodesValue<256>|DRW_ColorCodesValue<0>;

export interface DRW_SpaceValue<T extends number> {
  value: T;
}
export type DRW_Space = DRW_SpaceValue<0>|DRW_SpaceValue<1>;

export interface DRW_HandleCodesValue<T extends number> {
  value: T;
}
export type DRW_HandleCodes = DRW_HandleCodesValue<0>;

export interface DRW_ShadowModeValue<T extends number> {
  value: T;
}
export type DRW_ShadowMode = DRW_ShadowModeValue<0>|DRW_ShadowModeValue<1>|DRW_ShadowModeValue<2>|DRW_ShadowModeValue<3>;

export interface DRW_MaterialCodesValue<T extends number> {
  value: T;
}
export type DRW_MaterialCodes = DRW_MaterialCodesValue<0>;

export interface DRW_PlotStyleCodesValue<T extends number> {
  value: T;
}
export type DRW_PlotStyleCodes = DRW_PlotStyleCodesValue<0>;

export interface DRW_TransparencyCodesValue<T extends number> {
  value: T;
}
export type DRW_TransparencyCodes = DRW_TransparencyCodesValue<0>|DRW_TransparencyCodesValue<-1>;

export interface DRW_Coord extends ClassHandle {
  x: number;
  y: number;
  z: number;
  unitize(): void;
}

export interface DRW_Vertex2D extends ClassHandle {
  x: number;
  y: number;
  stawidth: number;
  endwidth: number;
  bulge: number;
}

export interface DRW_VariantTypeValue<T extends number> {
  value: T;
}
export type DRW_VariantType = DRW_VariantTypeValue<0>|DRW_VariantTypeValue<1>|DRW_VariantTypeValue<2>|DRW_VariantTypeValue<3>|DRW_VariantTypeValue<4>;

export interface DRW_Variant extends ClassHandle {
  addString(_0: number, _1: EmbindString): void;
  addInt(_0: number, _1: number): void;
  addDouble(_0: number, _1: number): void;
  addCoord(_0: number, _1: DRW_Coord): void;
  setCoordX(_0: number): void;
  setCoordY(_0: number): void;
  setCoordZ(_0: number): void;
  type(): DRW_VariantType;
  code(): number;
}

export interface DRW_dwgHandle extends ClassHandle {
  code: number;
  size: number;
  ref: number;
}

export interface DRW_LineWidthValue<T extends number> {
  value: T;
}
export type DRW_LineWidth = DRW_LineWidthValue<0>|DRW_LineWidthValue<1>|DRW_LineWidthValue<2>|DRW_LineWidthValue<3>|DRW_LineWidthValue<4>|DRW_LineWidthValue<5>|DRW_LineWidthValue<6>|DRW_LineWidthValue<7>|DRW_LineWidthValue<8>|DRW_LineWidthValue<9>|DRW_LineWidthValue<10>|DRW_LineWidthValue<11>|DRW_LineWidthValue<12>|DRW_LineWidthValue<13>|DRW_LineWidthValue<14>|DRW_LineWidthValue<15>|DRW_LineWidthValue<16>|DRW_LineWidthValue<17>|DRW_LineWidthValue<18>|DRW_LineWidthValue<19>|DRW_LineWidthValue<20>|DRW_LineWidthValue<21>|DRW_LineWidthValue<22>|DRW_LineWidthValue<23>|DRW_LineWidthValue<29>|DRW_LineWidthValue<30>|DRW_LineWidthValue<31>;

export interface DRW_LW_Conv extends ClassHandle {
}

export interface DRW_dbg_LevelValue<T extends number> {
  value: T;
}
export type DRW_dbg_Level = DRW_dbg_LevelValue<0>|DRW_dbg_LevelValue<1>;

export interface DRW_dbg extends ClassHandle {
  setLevel(_0: DRW_dbg_Level): void;
  getLevel(): DRW_dbg_Level;
  printString(_0: EmbindString): void;
  printInt(_0: number): void;
  printUnsignedInt(_0: number): void;
  printLongLongInt(_0: bigint): void;
  printLongUnsignedInt(_0: number): void;
  printLongLongUnsignedInt(_0: bigint): void;
  printDouble(_0: number): void;
  printH(_0: bigint): void;
  printB(_0: number): void;
  printHL(_0: number, _1: number, _2: number): void;
  printPT(_0: number, _1: number, _2: number): void;
}

export interface DRW_Header extends ClassHandle {
  addDouble(_0: EmbindString, _1: number, _2: number): void;
  addInt(_0: EmbindString, _1: number, _2: number): void;
  addStr(_0: EmbindString, _1: EmbindString, _2: number): void;
  addCoord(_0: EmbindString, _1: DRW_Coord, _2: number): void;
  getComments(): string;
  addComment(_0: EmbindString): void;
}

export interface DRW_VariantList extends ClassHandle {
  push_back(_0: DRW_Variant | null): void;
  resize(_0: number, _1: DRW_Variant | null): void;
  size(): number;
  get(_0: number): DRW_Variant | undefined;
  set(_0: number, _1: DRW_Variant | null): boolean;
}

export interface DRW_DoubleList extends ClassHandle {
  push_back(_0: number): void;
  resize(_0: number, _1: number): void;
  size(): number;
  get(_0: number): number | undefined;
  set(_0: number, _1: number): boolean;
}

export interface TTYPEValue<T extends number> {
  value: T;
}
export type TTYPE = TTYPEValue<0>|TTYPEValue<1>|TTYPEValue<2>|TTYPEValue<3>|TTYPEValue<4>|TTYPEValue<5>|TTYPEValue<6>|TTYPEValue<7>|TTYPEValue<8>;

export interface DRW_TableEntry extends ClassHandle {
  tType: TTYPE;
  handle: number;
  parentHandle: number;
  get name(): string;
  set name(value: EmbindString);
  flags: number;
  extData: DRW_VariantList;
}

export interface DRW_Dimstyle extends DRW_TableEntry {
  get dimpost(): string;
  set dimpost(value: EmbindString);
  get dimapost(): string;
  set dimapost(value: EmbindString);
  get dimblk(): string;
  set dimblk(value: EmbindString);
  get dimblk1(): string;
  set dimblk1(value: EmbindString);
  get dimblk2(): string;
  set dimblk2(value: EmbindString);
  dimscale: number;
  dimasz: number;
  dimexo: number;
  dimdli: number;
  dimexe: number;
  dimrnd: number;
  dimdle: number;
  dimtp: number;
  dimtm: number;
  dimfxl: number;
  dimtxt: number;
  dimcen: number;
  dimtsz: number;
  dimaltf: number;
  dimlfac: number;
  dimtvp: number;
  dimtfac: number;
  dimgap: number;
  dimaltrnd: number;
  dimtol: number;
  dimlim: number;
  dimtih: number;
  dimtoh: number;
  dimse1: number;
  dimse2: number;
  dimtad: number;
  dimzin: number;
  dimazin: number;
  dimalt: number;
  dimaltd: number;
  dimtofl: number;
  dimsah: number;
  dimtix: number;
  dimsoxd: number;
  dimclrd: number;
  dimclre: number;
  dimclrt: number;
  dimadec: number;
  dimunit: number;
  dimdec: number;
  dimtdec: number;
  dimaltu: number;
  dimalttd: number;
  dimaunit: number;
  dimfrac: number;
  dimlunit: number;
  dimdsep: number;
  dimtmove: number;
  dimjust: number;
  dimsd1: number;
  dimsd2: number;
  dimtolj: number;
  dimtzin: number;
  dimaltz: number;
  dimaltttz: number;
  dimfit: number;
  dimupt: number;
  dimatfit: number;
  dimfxlon: number;
  get dimtxsty(): string;
  set dimtxsty(value: EmbindString);
  get dimldrblk(): string;
  set dimldrblk(value: EmbindString);
  dimlwd: number;
  dimlwe: number;
  reset(): void;
}

export interface DRW_LType extends DRW_TableEntry {
  get desc(): string;
  set desc(value: EmbindString);
  size: number;
  length: number;
  path: DRW_DoubleList;
  reset(): void;
}

export interface DRW_Layer extends DRW_TableEntry {
  get lineType(): string;
  set lineType(value: EmbindString);
  color: number;
  color24: number;
  plotF: boolean;
  lWeight: DRW_LineWidth;
  get handlePlotS(): string;
  set handlePlotS(value: EmbindString);
  get handleMaterialS(): string;
  set handleMaterialS(value: EmbindString);
  lTypeH: DRW_dwgHandle;
  reset(): void;
}

export interface DRW_Block_Record extends DRW_TableEntry {
  insUnits: number;
  basePoint: DRW_Coord;
  reset(): void;
}

export interface DRW_Textstyle extends DRW_TableEntry {
  height: number;
  width: number;
  oblique: number;
  genFlag: number;
  lastHeight: number;
  get font(): string;
  set font(value: EmbindString);
  get bigFont(): string;
  set bigFont(value: EmbindString);
  fontFamily: number;
  reset(): void;
}

export interface DRW_Vport extends DRW_TableEntry {
  lowerLeft: DRW_Coord;
  UpperRight: DRW_Coord;
  center: DRW_Coord;
  snapBase: DRW_Coord;
  snapSpacing: DRW_Coord;
  gridSpacing: DRW_Coord;
  viewDir: DRW_Coord;
  viewTarget: DRW_Coord;
  height: number;
  ratio: number;
  lensHeight: number;
  frontClip: number;
  backClip: number;
  snapAngle: number;
  twistAngle: number;
  viewMode: number;
  circleZoom: number;
  fastZoom: number;
  ucsIcon: number;
  snap: number;
  grid: number;
  snapStyle: number;
  snapIsopair: number;
  gridBehavior: number;
  reset(): void;
}

export interface DRW_ImageDef extends DRW_TableEntry {
  get name(): string;
  set name(value: EmbindString);
  imgVersion: number;
  u: number;
  v: number;
  up: number;
  vp: number;
  loaded: number;
  resolution: number;
  reset(): void;
}

export interface DRW_AppId extends DRW_TableEntry {
  reset(): void;
}

export type DRW_VariantPtr = {

};

export type DRW_VertexPtr = {

};

export type DRW_Vertex2DPtr = {

};

export type DRW_HatchLoopPtr = {

};

export interface DRW_VertexList extends ClassHandle {
  size(): number;
  get(_0: number): DRW_Vertex | undefined;
  push_back(_0: DRW_Vertex | null): void;
  resize(_0: number, _1: DRW_Vertex | null): void;
  set(_0: number, _1: DRW_Vertex | null): boolean;
}

export interface DRW_Vertex2DList extends ClassHandle {
  push_back(_0: DRW_Vertex2D | null): void;
  resize(_0: number, _1: DRW_Vertex2D | null): void;
  size(): number;
  get(_0: number): DRW_Vertex2D | undefined;
  set(_0: number, _1: DRW_Vertex2D | null): boolean;
}

export interface DRW_CoordList extends ClassHandle {
  push_back(_0: DRW_Coord | null): void;
  resize(_0: number, _1: DRW_Coord | null): void;
  size(): number;
  get(_0: number): DRW_Coord | undefined;
  set(_0: number, _1: DRW_Coord | null): boolean;
}

export interface DRW_HatchLoopList extends ClassHandle {
  size(): number;
  get(_0: number): DRW_HatchLoop | undefined;
  push_back(_0: DRW_HatchLoop | null): void;
  resize(_0: number, _1: DRW_HatchLoop | null): void;
  set(_0: number, _1: DRW_HatchLoop | null): boolean;
}

export interface DRW_ETYPEValue<T extends number> {
  value: T;
}
export type DRW_ETYPE = DRW_ETYPEValue<0>|DRW_ETYPEValue<1>|DRW_ETYPEValue<2>|DRW_ETYPEValue<3>|DRW_ETYPEValue<4>|DRW_ETYPEValue<5>|DRW_ETYPEValue<6>|DRW_ETYPEValue<7>|DRW_ETYPEValue<8>|DRW_ETYPEValue<9>|DRW_ETYPEValue<10>|DRW_ETYPEValue<11>|DRW_ETYPEValue<12>|DRW_ETYPEValue<13>|DRW_ETYPEValue<14>|DRW_ETYPEValue<15>|DRW_ETYPEValue<16>|DRW_ETYPEValue<17>|DRW_ETYPEValue<18>|DRW_ETYPEValue<19>|DRW_ETYPEValue<20>|DRW_ETYPEValue<21>|DRW_ETYPEValue<22>|DRW_ETYPEValue<23>|DRW_ETYPEValue<24>|DRW_ETYPEValue<25>|DRW_ETYPEValue<26>|DRW_ETYPEValue<27>|DRW_ETYPEValue<28>|DRW_ETYPEValue<29>|DRW_ETYPEValue<30>|DRW_ETYPEValue<31>;

export interface DRW_Entity extends ClassHandle {
  eType: DRW_ETYPE;
  handle: number;
  parentHandle: number;
  space: DRW_Space;
  get layer(): string;
  set layer(value: EmbindString);
  get lineType(): string;
  set lineType(value: EmbindString);
  material: number;
  color: number;
  lWeight: DRW_LineWidth;
  ltypeScale: number;
  visible: boolean;
  numProxyGraph: number;
  get proxyGraphics(): string;
  set proxyGraphics(value: EmbindString);
  color24: number;
  get colorName(): string;
  set colorName(value: EmbindString);
  transparency: number;
  plotStyle: number;
  shadow: DRW_ShadowMode;
  haveExtrusion: boolean;
  reset(): void;
  applyExtrusion(): void;
}

export interface DRW_Point extends DRW_Entity {
  basePoint: DRW_Coord;
  thickness: number;
  extPoint: DRW_Coord;
  applyExtrusion(): void;
  applyExtrusion(): void;
  applyExtrusion(): void;
  applyExtrusion(): void;
  applyExtrusion(): void;
}

export interface DRW_Line extends DRW_Point {
  secPoint: DRW_Coord;
}

export interface DRW_Ray extends DRW_Line {
}

export interface DRW_Xline extends DRW_Ray {
}

export interface DRW_Circle extends DRW_Point {
  radious: number;
  applyExtrusion(): void;
}

export interface DRW_Arc extends DRW_Circle {
  staangle: number;
  endangle: number;
  isccw: number;
  applyExtrusion(): void;
  center(): DRW_Coord;
  radius(): number;
  startAngle(): number;
  endAngle(): number;
  thick(): number;
  extrusion(): DRW_Coord;
}

export interface DRW_Ellipse extends DRW_Line {
  ratio: number;
  staparam: number;
  endparam: number;
  isccw: number;
  applyExtrusion(): void;
  toPolyline(_0: DRW_Polyline | null, _1: number): void;
}

export interface DRW_Trace extends DRW_Line {
  thirdPoint: DRW_Coord;
  fourPoint: DRW_Coord;
  applyExtrusion(): void;
  applyExtrusion(): void;
}

export interface DRW_Solid extends DRW_Trace {
  firstCorner(): DRW_Coord;
  secondCorner(): DRW_Coord;
  thirdCorner(): DRW_Coord;
  fourthCorner(): DRW_Coord;
  thick(): number;
  elevation(): number;
  extrusion(): DRW_Coord;
}

export interface InvisibleEdgeFlagsValue<T extends number> {
  value: T;
}
export type InvisibleEdgeFlags = InvisibleEdgeFlagsValue<0>|InvisibleEdgeFlagsValue<1>|InvisibleEdgeFlagsValue<2>|InvisibleEdgeFlagsValue<4>|InvisibleEdgeFlagsValue<8>|InvisibleEdgeFlagsValue<15>;

export interface DRW_3Dface extends DRW_Trace {
  invisibleflag: number;
  firstCorner(): DRW_Coord;
  secondCorner(): DRW_Coord;
  thirdCorner(): DRW_Coord;
  fourthCorner(): DRW_Coord;
  edgeFlags(): InvisibleEdgeFlags;
}

export interface DRW_Block extends DRW_Point {
  get name(): string;
  set name(value: EmbindString);
  flags: number;
}

export interface DRW_Insert extends DRW_Point {
  get name(): string;
  set name(value: EmbindString);
  xscale: number;
  yscale: number;
  zscale: number;
  angle: number;
  colcount: number;
  rowcount: number;
  colspace: number;
  rowspace: number;
  blockRecH: DRW_dwgHandle;
  seqendH: DRW_dwgHandle;
}

export interface DRW_LWPolyline extends DRW_Entity {
  vertexnum: number;
  flags: number;
  width: number;
  elevation: number;
  thickness: number;
  extPoint: DRW_Coord;
  vertex: DRW_Vertex2DPtr;
  applyExtrusion(): void;
  addVertex(_0: DRW_Vertex2D): void;
  appendVertex(): DRW_Vertex2DPtr;
  getVertexList(): DRW_Vertex2DList;
}

export interface VAlignValue<T extends number> {
  value: T;
}
export type VAlign = VAlignValue<0>|VAlignValue<1>|VAlignValue<2>|VAlignValue<3>;

export interface HAlignValue<T extends number> {
  value: T;
}
export type HAlign = HAlignValue<0>|HAlignValue<1>|HAlignValue<2>|HAlignValue<3>|HAlignValue<4>|HAlignValue<5>;

export interface DRW_Text extends DRW_Entity {
  height: number;
  get text(): string;
  set text(value: EmbindString);
  angle: number;
  widthscale: number;
  oblique: number;
  get style(): string;
  set style(value: EmbindString);
  textgen: number;
  alignH: HAlign;
  alignV: VAlign;
  styleH: DRW_dwgHandle;
  applyExtrusion(): void;
}

export interface AttachValue<T extends number> {
  value: T;
}
export type Attach = AttachValue<1>|AttachValue<2>|AttachValue<3>|AttachValue<4>|AttachValue<5>|AttachValue<6>|AttachValue<7>|AttachValue<8>|AttachValue<9>;

export interface DRW_MText extends DRW_Text {
  interlin: number;
}

export interface DRW_Vertex extends DRW_Point {
  stawidth: number;
  endwidth: number;
  bulge: number;
  flags: number;
  tgdir: number;
  vindex1: number;
  vindex2: number;
  vindex3: number;
  vindex4: number;
  identifier: number;
}

export interface DRW_Polyline extends DRW_Point {
  defstawidth: number;
  defendwidth: number;
  flags: number;
  vertexcount: number;
  facecount: number;
  smoothM: number;
  smoothN: number;
  curvetype: number;
  addVertex(_0: DRW_Vertex): void;
  appendVertex(_0: DRW_VertexPtr): void;
  getVertexList(): DRW_VertexList;
  addVertex(_0: DRW_Vertex): void;
  appendVertex(_0: DRW_VertexPtr): void;
}

export interface DRW_Spline extends DRW_Entity {
  normalVec: DRW_Coord;
  tgStart: DRW_Coord;
  tgEnd: DRW_Coord;
  flags: number;
  degree: number;
  nknots: number;
  ncontrol: number;
  nfit: number;
  tolknot: number;
  tolcontrol: number;
  tolfit: number;
  knotslist: DRW_DoubleList;
  applyExtrusion(): void;
  getControlList(): DRW_CoordList;
  getFitList(): DRW_CoordList;
}

export interface DRW_HatchLoop extends ClassHandle {
  type: number;
  numedges: number;
  update(): void;
}

export interface DRW_Hatch extends DRW_Point {
  get name(): string;
  set name(value: EmbindString);
  solid: number;
  associative: number;
  hstyle: number;
  hpattern: number;
  doubleflag: number;
  loopsnum: number;
  angle: number;
  scale: number;
  deflines: number;
  appendLoop(_0: DRW_HatchLoopPtr): void;
  getLoopList(): DRW_HatchLoopList;
}

export interface DRW_Image extends DRW_Line {
  ref: number;
  vVector: DRW_Coord;
  sizeu: number;
  sizev: number;
  dz: number;
  clip: number;
  brightness: number;
  contrast: number;
  fade: number;
}

export interface DRW_Dimension extends DRW_Entity {
  getDefPoint(): DRW_Coord;
  setDefPoint(_0: DRW_Coord): void;
  getTextPoint(): DRW_Coord;
  setTextPoint(_0: DRW_Coord): void;
  getStyle(): string;
  setStyle(_0: EmbindString): void;
  getAlign(): number;
  setAlign(_0: number): void;
  getTextLineStyle(): number;
  setTextLineStyle(_0: number): void;
  getText(): string;
  setText(_0: EmbindString): void;
  getTextLineFactor(): number;
  setTextLineFactor(_0: number): void;
  getDir(): number;
  setDir(_0: number): void;
  getExtrusion(): DRW_Coord;
  setExtrusion(_0: DRW_Coord): void;
  getName(): string;
  setName(_0: EmbindString): void;
}

export interface DRW_DimAligned extends DRW_Dimension {
  getClonePoint(): DRW_Coord;
  setClonePoint(_0: DRW_Coord): void;
  getDimPoint(): DRW_Coord;
  setDimPoint(_0: DRW_Coord): void;
  getDef1Point(): DRW_Coord;
  setDef1Point(_0: DRW_Coord): void;
  getDef2Point(): DRW_Coord;
  setDef2Point(_0: DRW_Coord): void;
}

export interface DRW_DimLinear extends DRW_DimAligned {
  getAngle(): number;
  setAngle(_0: number): void;
  getOblique(): number;
  setOblique(_0: number): void;
}

export interface DRW_DimRadial extends DRW_Dimension {
  getCenterPoint(): DRW_Coord;
  setCenterPoint(_0: DRW_Coord): void;
  getDiameterPoint(): DRW_Coord;
  setDiameterPoint(_0: DRW_Coord): void;
  getLeaderLength(): number;
  setLeaderLength(_0: number): void;
}

export interface DRW_DimDiametric extends DRW_Dimension {
  getDiameter1Point(): DRW_Coord;
  setDiameter1Point(_0: DRW_Coord): void;
  getDiameter2Point(): DRW_Coord;
  setDiameter2Point(_0: DRW_Coord): void;
  getLeaderLength(): number;
  setLeaderLength(_0: number): void;
}

export interface DRW_DimAngular extends DRW_Dimension {
  getFirstLine1(): DRW_Coord;
  setFirstLine1(_0: DRW_Coord): void;
  getFirstLine2(): DRW_Coord;
  setFirstLine2(_0: DRW_Coord): void;
  getSecondLine1(): DRW_Coord;
  setSecondLine1(_0: DRW_Coord): void;
  getSecondLine2(): DRW_Coord;
  setSecondLine2(_0: DRW_Coord): void;
  getDimPoint(): DRW_Coord;
  setDimPoint(_0: DRW_Coord): void;
}

export interface DRW_DimAngular3p extends DRW_Dimension {
  getFirstLine(): DRW_Coord;
  setFirstLine(_0: DRW_Coord): void;
  getSecondLine(): DRW_Coord;
  setSecondLine(_0: DRW_Coord): void;
  getVertexPoint(): DRW_Coord;
  SetVertexPoint(_0: DRW_Coord): void;
  getDimPoint(): DRW_Coord;
  setDimPoint(_0: DRW_Coord): void;
}

export interface DRW_DimOrdinate extends DRW_Dimension {
  getOriginPoint(): DRW_Coord;
  setOriginPoint(_0: DRW_Coord): void;
  getFirstLine(): DRW_Coord;
  setFirstLine(_0: DRW_Coord): void;
  getSecondLine(): DRW_Coord;
  setSecondLine(_0: DRW_Coord): void;
}

export interface DRW_Leader extends DRW_Entity {
  get style(): string;
  set style(value: EmbindString);
  arrow: number;
  leadertype: number;
  flag: number;
  hookline: number;
  hookflag: number;
  textheight: number;
  textwidth: number;
  vertnum: number;
  coloruse: number;
  annotHandle: number;
  extrusionPoint: DRW_Coord;
  horizdir: DRW_Coord;
  offsetblock: DRW_Coord;
  offsettext: DRW_Coord;
  applyExtrusion(): void;
}

export interface DRW_Viewport extends DRW_Point {
  pswidth: number;
  psheight: number;
  vpstatus: number;
  vpID: number;
  centerPX: number;
  centerPY: number;
  snapPX: number;
  snapPY: number;
  snapSpPX: number;
  snapSpPY: number;
  viewDir: DRW_Coord;
  viewTarget: DRW_Coord;
  viewLength: number;
  frontClip: number;
  backClip: number;
  viewHeight: number;
  snapAngle: number;
  twistAngle: number;
}

export interface DRW_Interface extends ClassHandle {
  addHeader(_0: DRW_Header | null): void;
  addLType(_0: DRW_LType): void;
  addLayer(_0: DRW_Layer): void;
  addDimStyle(_0: DRW_Dimstyle): void;
  addVport(_0: DRW_Vport): void;
  addTextStyle(_0: DRW_Textstyle): void;
  addAppId(_0: DRW_AppId): void;
  addBlock(_0: DRW_Block): void;
  setBlock(_0: number): void;
  endBlock(): void;
  addPoint(_0: DRW_Point): void;
  addLine(_0: DRW_Line): void;
  addRay(_0: DRW_Ray): void;
  addXline(_0: DRW_Xline): void;
  addArc(_0: DRW_Arc): void;
  addCircle(_0: DRW_Circle): void;
  addEllipse(_0: DRW_Ellipse): void;
  addLWPolyline(_0: DRW_LWPolyline): void;
  addPolyline(_0: DRW_Polyline): void;
  addSpline(_0: DRW_Spline | null): void;
  addKnot(_0: DRW_Entity): void;
  addInsert(_0: DRW_Insert): void;
  addTrace(_0: DRW_Trace): void;
  add3dFace(_0: DRW_3Dface): void;
  addSolid(_0: DRW_Solid): void;
  addMText(_0: DRW_MText): void;
  addText(_0: DRW_Text): void;
  addDimAlign(_0: DRW_DimAligned | null): void;
  addDimLinear(_0: DRW_DimLinear | null): void;
  addDimRadial(_0: DRW_DimRadial | null): void;
  addDimDiametric(_0: DRW_DimDiametric | null): void;
  addDimAngular(_0: DRW_DimAngular | null): void;
  addDimAngular3P(_0: DRW_DimAngular3p | null): void;
  addDimOrdinate(_0: DRW_DimOrdinate | null): void;
  addLeader(_0: DRW_Leader | null): void;
  addHatch(_0: DRW_Hatch | null): void;
  addViewport(_0: DRW_Viewport): void;
  addImage(_0: DRW_Image | null): void;
  linkImage(_0: DRW_ImageDef | null): void;
  writeHeader(_0: DRW_Header): void;
  writeBlocks(): void;
  writeBlockRecords(): void;
  writeEntities(): void;
  writeLTypes(): void;
  writeLayers(): void;
  writeTextstyles(): void;
  writeVports(): void;
  writeDimstyles(): void;
  writeAppId(): void;
}

export interface dxfRW extends ClassHandle {
  setDebug(_0: DRW_DebugLevel): void;
  read(_0: DRW_Interface | null, _1: boolean): boolean;
  setBinary(_0: boolean): void;
  write(_0: DRW_Interface | null, _1: DRW_Version, _2: boolean): string;
  writeLineType(_0: DRW_LType | null): boolean;
  writeLayer(_0: DRW_Layer | null): boolean;
  writeDimstyle(_0: DRW_Dimstyle | null): boolean;
  writeTextstyle(_0: DRW_Textstyle | null): boolean;
  writeVport(_0: DRW_Vport | null): boolean;
  writeAppId(_0: DRW_AppId | null): boolean;
  writePoint(_0: DRW_Point | null): boolean;
  writeLine(_0: DRW_Line | null): boolean;
  writeRay(_0: DRW_Ray | null): boolean;
  writeXline(_0: DRW_Xline | null): boolean;
  writeCircle(_0: DRW_Circle | null): boolean;
  writeArc(_0: DRW_Arc | null): boolean;
  writeEllipse(_0: DRW_Ellipse | null): boolean;
  writeTrace(_0: DRW_Trace | null): boolean;
  writeSolid(_0: DRW_Solid | null): boolean;
  write3dface(_0: DRW_3Dface | null): boolean;
  writeLWPolyline(_0: DRW_LWPolyline | null): boolean;
  writePolyline(_0: DRW_Polyline | null): boolean;
  writeSpline(_0: DRW_Spline | null): boolean;
  writeBlockRecord(_0: EmbindString): boolean;
  writeBlock(_0: DRW_Block | null): boolean;
  writeInsert(_0: DRW_Insert | null): boolean;
  writeMText(_0: DRW_MText | null): boolean;
  writeText(_0: DRW_Text | null): boolean;
  writeHatch(_0: DRW_Hatch | null): boolean;
  writeViewport(_0: DRW_Viewport | null): boolean;
  writeImage(_0: DRW_Image | null, _1: EmbindString): DRW_ImageDef | null;
  writeLeader(_0: DRW_Leader | null): boolean;
  writeDimension(_0: DRW_Dimension | null): boolean;
  setEllipseParts(_0: number): void;
}

export interface dwgR extends ClassHandle {
  read(_0: DRW_Interface | null, _1: boolean): boolean;
  getPreview(): boolean;
  getVersion(): DRW_Version;
  getError(): DWR_Error;
  testReader(): boolean;
  setDebug(_0: DRW_DebugLevel): void;
}

export interface DRW_EntityList extends ClassHandle {
  push_back(_0: DRW_Entity | null): void;
  resize(_0: number, _1: DRW_Entity | null): void;
  size(): number;
  get(_0: number): DRW_Entity | undefined;
  set(_0: number, _1: DRW_Entity | null): boolean;
}

export interface DRW_LTypeList extends ClassHandle {
  push_back(_0: DRW_LType): void;
  resize(_0: number, _1: DRW_LType): void;
  size(): number;
  get(_0: number): DRW_LType | undefined;
  set(_0: number, _1: DRW_LType): boolean;
}

export interface DRW_LayerList extends ClassHandle {
  push_back(_0: DRW_Layer): void;
  resize(_0: number, _1: DRW_Layer): void;
  size(): number;
  get(_0: number): DRW_Layer | undefined;
  set(_0: number, _1: DRW_Layer): boolean;
}

export interface DRW_DimstyleList extends ClassHandle {
  push_back(_0: DRW_Dimstyle): void;
  resize(_0: number, _1: DRW_Dimstyle): void;
  size(): number;
  get(_0: number): DRW_Dimstyle | undefined;
  set(_0: number, _1: DRW_Dimstyle): boolean;
}

export interface DRW_VportList extends ClassHandle {
  push_back(_0: DRW_Vport): void;
  resize(_0: number, _1: DRW_Vport): void;
  size(): number;
  get(_0: number): DRW_Vport | undefined;
  set(_0: number, _1: DRW_Vport): boolean;
}

export interface DRW_TextstyleList extends ClassHandle {
  push_back(_0: DRW_Textstyle): void;
  resize(_0: number, _1: DRW_Textstyle): void;
  size(): number;
  get(_0: number): DRW_Textstyle | undefined;
  set(_0: number, _1: DRW_Textstyle): boolean;
}

export interface DRW_AppIdList extends ClassHandle {
  push_back(_0: DRW_AppId): void;
  resize(_0: number, _1: DRW_AppId): void;
  size(): number;
  get(_0: number): DRW_AppId | undefined;
  set(_0: number, _1: DRW_AppId): boolean;
}

export interface DRW_ifaceBlockList extends ClassHandle {
  size(): number;
  get(_0: number): dx_ifaceBlock | undefined;
  push_back(_0: dx_ifaceBlock | null): void;
  resize(_0: number, _1: dx_ifaceBlock | null): void;
  set(_0: number, _1: dx_ifaceBlock | null): boolean;
}

export interface DRW_ifaceImgList extends ClassHandle {
  size(): number;
  get(_0: number): dx_ifaceImg | undefined;
  push_back(_0: dx_ifaceImg | null): void;
  resize(_0: number, _1: dx_ifaceImg | null): void;
  set(_0: number, _1: dx_ifaceImg | null): boolean;
}

export interface dx_ifaceImg extends ClassHandle {
  get path(): string;
  set path(value: EmbindString);
}

export interface dx_ifaceBlock extends DRW_Block {
  ent: DRW_EntityList;
}

export interface dx_data extends ClassHandle {
  headerC: DRW_Header;
  lineTypes: DRW_LTypeList;
  layers: DRW_LayerList;
  dimStyles: DRW_DimstyleList;
  viewports: DRW_VportList;
  textStyles: DRW_TextstyleList;
  appIds: DRW_AppIdList;
  blocks: DRW_ifaceBlockList;
  images: DRW_ifaceImgList;
  mBlock: dx_ifaceBlock | null;
}

export interface dx_iface extends DRW_Interface {
  dxfW: dxfRW | null;
  cData: dx_data | null;
  currentBlock: dx_ifaceBlock | null;
  fileImport(_0: EmbindString, _1: dx_data | null, _2: boolean, _3: boolean): boolean;
  fileExport(_0: DRW_Version, _1: boolean, _2: dx_data | null, _3: boolean): string;
  writeEntity(_0: DRW_Entity | null): void;
  addHeader(_0: DRW_Header | null): void;
  addLType(_0: DRW_LType): void;
  addLayer(_0: DRW_Layer): void;
  addDimStyle(_0: DRW_Dimstyle): void;
  addVport(_0: DRW_Vport): void;
  addTextStyle(_0: DRW_Textstyle): void;
  addAppId(_0: DRW_AppId): void;
  addBlock(_0: DRW_Block): void;
  setBlock(_0: number): void;
  endBlock(): void;
  addPoint(_0: DRW_Point): void;
  addLine(_0: DRW_Line): void;
  addRay(_0: DRW_Ray): void;
  addXline(_0: DRW_Xline): void;
  addArc(_0: DRW_Arc): void;
  addCircle(_0: DRW_Circle): void;
  addEllipse(_0: DRW_Ellipse): void;
  addLWPolyline(_0: DRW_LWPolyline): void;
  addPolyline(_0: DRW_Polyline): void;
  addSpline(_0: DRW_Spline | null): void;
  addKnot(_0: DRW_Entity): void;
  addInsert(_0: DRW_Insert): void;
  addTrace(_0: DRW_Trace): void;
  add3dFace(_0: DRW_3Dface): void;
  addSolid(_0: DRW_Solid): void;
  addMText(_0: DRW_MText): void;
  addText(_0: DRW_Text): void;
  addDimAlign(_0: DRW_DimAligned | null): void;
  addDimLinear(_0: DRW_DimLinear | null): void;
  addDimRadial(_0: DRW_DimRadial | null): void;
  addDimDiametric(_0: DRW_DimDiametric | null): void;
  addDimAngular(_0: DRW_DimAngular | null): void;
  addDimAngular3P(_0: DRW_DimAngular3p | null): void;
  addDimOrdinate(_0: DRW_DimOrdinate | null): void;
  addLeader(_0: DRW_Leader | null): void;
  addHatch(_0: DRW_Hatch | null): void;
  addViewport(_0: DRW_Viewport): void;
  addImage(_0: DRW_Image | null): void;
  linkImage(_0: DRW_ImageDef | null): void;
  writeHeader(_0: DRW_Header): void;
  writeBlocks(): void;
  writeBlockRecords(): void;
  writeEntities(): void;
  writeLTypes(): void;
  writeLayers(): void;
  writeTextstyles(): void;
  writeVports(): void;
  writeDimstyles(): void;
  writeAppId(): void;
}

interface EmbindModule {
  dxfWriter: {};
  DRW_Version: {UNKNOWNV: DRW_VersionValue<0>, MC00: DRW_VersionValue<1>, AC12: DRW_VersionValue<2>, AC14: DRW_VersionValue<3>, AC150: DRW_VersionValue<4>, AC210: DRW_VersionValue<5>, AC1002: DRW_VersionValue<6>, AC1003: DRW_VersionValue<7>, AC1004: DRW_VersionValue<8>, AC1006: DRW_VersionValue<9>, AC1009: DRW_VersionValue<10>, AC1012: DRW_VersionValue<11>, AC1014: DRW_VersionValue<12>, AC1015: DRW_VersionValue<13>, AC1018: DRW_VersionValue<14>, AC1021: DRW_VersionValue<15>, AC1024: DRW_VersionValue<16>, AC1027: DRW_VersionValue<17>, AC1032: DRW_VersionValue<18>};
  DWR_Error: {BAD_NONE: DWR_ErrorValue<0>, BAD_UNKNOWN: DWR_ErrorValue<1>, BAD_OPEN: DWR_ErrorValue<2>, BAD_VERSION: DWR_ErrorValue<3>, BAD_READ_METADATA: DWR_ErrorValue<4>, BAD_READ_FILE_HEADER: DWR_ErrorValue<5>, BAD_READ_HEADER: DWR_ErrorValue<6>, BAD_READ_HANDLES: DWR_ErrorValue<7>, BAD_READ_CLASSES: DWR_ErrorValue<8>, BAD_READ_TABLES: DWR_ErrorValue<9>, BAD_READ_BLOCKS: DWR_ErrorValue<10>, BAD_READ_ENTITIES: DWR_ErrorValue<11>, BAD_READ_OBJECTS: DWR_ErrorValue<12>, BAD_READ_SECTION: DWR_ErrorValue<13>, BAD_CODE_PARSED: DWR_ErrorValue<14>};
  DRW_DebugLevel: {None: DRW_DebugLevelValue<0>, Debug: DRW_DebugLevelValue<1>};
  DebugPrinter: {
    new(): DebugPrinter;
  };
  DRW_ColorCodes: {ColorByLayer: DRW_ColorCodesValue<256>, ColorByBlock: DRW_ColorCodesValue<0>};
  DRW_Space: {ModelSpace: DRW_SpaceValue<0>, PaperSpace: DRW_SpaceValue<1>};
  DRW_HandleCodes: {NoHandle: DRW_HandleCodesValue<0>};
  DRW_ShadowMode: {CastAndReceieveShadows: DRW_ShadowModeValue<0>, CastShadows: DRW_ShadowModeValue<1>, ReceiveShadows: DRW_ShadowModeValue<2>, IgnoreShadows: DRW_ShadowModeValue<3>};
  DRW_MaterialCodes: {MaterialByLayer: DRW_MaterialCodesValue<0>};
  DRW_PlotStyleCodes: {DefaultPlotStyle: DRW_PlotStyleCodesValue<0>};
  DRW_TransparencyCodes: {Opaque: DRW_TransparencyCodesValue<0>, Transparent: DRW_TransparencyCodesValue<-1>};
  DRW_Coord: {
    new(): DRW_Coord;
    new(_0: number, _1: number, _2: number): DRW_Coord;
  };
  DRW_Vertex2D: {
    new(): DRW_Vertex2D;
    new(_0: number, _1: number, _2: number): DRW_Vertex2D;
  };
  DRW_VariantType: {STRING: DRW_VariantTypeValue<0>, INTEGER: DRW_VariantTypeValue<1>, DOUBLE: DRW_VariantTypeValue<2>, COORD: DRW_VariantTypeValue<3>, INVALID: DRW_VariantTypeValue<4>};
  DRW_Variant: {
    new(): DRW_Variant;
  };
  DRW_dwgHandle: {
    new(): DRW_dwgHandle;
  };
  DRW_LineWidth: {width00: DRW_LineWidthValue<0>, width01: DRW_LineWidthValue<1>, width02: DRW_LineWidthValue<2>, width03: DRW_LineWidthValue<3>, width04: DRW_LineWidthValue<4>, width05: DRW_LineWidthValue<5>, width06: DRW_LineWidthValue<6>, width07: DRW_LineWidthValue<7>, width08: DRW_LineWidthValue<8>, width09: DRW_LineWidthValue<9>, width10: DRW_LineWidthValue<10>, width11: DRW_LineWidthValue<11>, width12: DRW_LineWidthValue<12>, width13: DRW_LineWidthValue<13>, width14: DRW_LineWidthValue<14>, width15: DRW_LineWidthValue<15>, width16: DRW_LineWidthValue<16>, width17: DRW_LineWidthValue<17>, width18: DRW_LineWidthValue<18>, width19: DRW_LineWidthValue<19>, width20: DRW_LineWidthValue<20>, width21: DRW_LineWidthValue<21>, width22: DRW_LineWidthValue<22>, width23: DRW_LineWidthValue<23>, widthByLayer: DRW_LineWidthValue<29>, widthByBlock: DRW_LineWidthValue<30>, widthDefault: DRW_LineWidthValue<31>};
  DRW_LW_Conv: {
    lineWidth2dxfInt(_0: DRW_LineWidth): number;
    lineWidth2dwgInt(_0: DRW_LineWidth): number;
    dxfInt2lineWidth(_0: number): DRW_LineWidth;
    dwgInt2lineWidth(_0: number): DRW_LineWidth;
  };
  DRW_dbg_Level: {None: DRW_dbg_LevelValue<0>, Debug: DRW_dbg_LevelValue<1>};
  DRW_dbg: {
    getInstance(): DRW_dbg | null;
  };
  DRW_Header: {
    new(): DRW_Header;
  };
  DRW_VariantList: {
    new(): DRW_VariantList;
  };
  DRW_DoubleList: {
    new(): DRW_DoubleList;
  };
  TTYPE: {UNKNOWNT: TTYPEValue<0>, LTYPE: TTYPEValue<1>, LAYER: TTYPEValue<2>, STYLE: TTYPEValue<3>, DIMSTYLE: TTYPEValue<4>, VPORT: TTYPEValue<5>, BLOCK_RECORD: TTYPEValue<6>, APPID: TTYPEValue<7>, IMAGEDEF: TTYPEValue<8>};
  DRW_TableEntry: {};
  DRW_Dimstyle: {
    new(): DRW_Dimstyle;
  };
  DRW_LType: {
    new(): DRW_LType;
  };
  DRW_Layer: {
    new(): DRW_Layer;
  };
  DRW_Block_Record: {
    new(): DRW_Block_Record;
  };
  DRW_Textstyle: {
    new(): DRW_Textstyle;
  };
  DRW_Vport: {
    new(): DRW_Vport;
  };
  DRW_ImageDef: {
    new(): DRW_ImageDef;
  };
  DRW_AppId: {
    new(): DRW_AppId;
  };
  DRW_VertexList: {
    new(): DRW_VertexList;
  };
  DRW_Vertex2DList: {
    new(): DRW_Vertex2DList;
  };
  DRW_CoordList: {
    new(): DRW_CoordList;
  };
  DRW_HatchLoopList: {
    new(): DRW_HatchLoopList;
  };
  DRW_ETYPE: {E3DFACE: DRW_ETYPEValue<0>, ARC: DRW_ETYPEValue<1>, BLOCK: DRW_ETYPEValue<2>, CIRCLE: DRW_ETYPEValue<3>, DIMENSION: DRW_ETYPEValue<4>, DIMALIGNED: DRW_ETYPEValue<5>, DIMLINEAR: DRW_ETYPEValue<6>, DIMRADIAL: DRW_ETYPEValue<7>, DIMDIAMETRIC: DRW_ETYPEValue<8>, DIMANGULAR: DRW_ETYPEValue<9>, DIMANGULAR3P: DRW_ETYPEValue<10>, DIMORDINATE: DRW_ETYPEValue<11>, ELLIPSE: DRW_ETYPEValue<12>, HATCH: DRW_ETYPEValue<13>, IMAGE: DRW_ETYPEValue<14>, INSERT: DRW_ETYPEValue<15>, LEADER: DRW_ETYPEValue<16>, LINE: DRW_ETYPEValue<17>, LWPOLYLINE: DRW_ETYPEValue<18>, MTEXT: DRW_ETYPEValue<19>, POINT: DRW_ETYPEValue<20>, POLYLINE: DRW_ETYPEValue<21>, RAY: DRW_ETYPEValue<22>, SOLID: DRW_ETYPEValue<23>, SPLINE: DRW_ETYPEValue<24>, TEXT: DRW_ETYPEValue<25>, TRACE: DRW_ETYPEValue<26>, UNDERLAY: DRW_ETYPEValue<27>, VERTEX: DRW_ETYPEValue<28>, VIEWPORT: DRW_ETYPEValue<29>, XLINE: DRW_ETYPEValue<30>, UNKNOWN: DRW_ETYPEValue<31>};
  DRW_Entity: {};
  DRW_Point: {
    new(): DRW_Point;
  };
  DRW_Line: {
    new(): DRW_Line;
  };
  DRW_Ray: {
    new(): DRW_Ray;
  };
  DRW_Xline: {
    new(): DRW_Xline;
  };
  DRW_Circle: {
    new(): DRW_Circle;
  };
  DRW_Arc: {
    new(): DRW_Arc;
  };
  DRW_Ellipse: {
    new(): DRW_Ellipse;
  };
  DRW_Trace: {
    new(): DRW_Trace;
  };
  DRW_Solid: {
    new(): DRW_Solid;
  };
  InvisibleEdgeFlags: {NoEdge: InvisibleEdgeFlagsValue<0>, FirstEdge: InvisibleEdgeFlagsValue<1>, SecondEdge: InvisibleEdgeFlagsValue<2>, ThirdEdge: InvisibleEdgeFlagsValue<4>, FourthEdge: InvisibleEdgeFlagsValue<8>, AllEdges: InvisibleEdgeFlagsValue<15>};
  DRW_3Dface: {
    new(): DRW_3Dface;
  };
  DRW_Block: {
    new(): DRW_Block;
  };
  DRW_Insert: {
    new(): DRW_Insert;
  };
  DRW_LWPolyline: {
    new(): DRW_LWPolyline;
  };
  VAlign: {VBaseLine: VAlignValue<0>, VBottom: VAlignValue<1>, VMiddle: VAlignValue<2>, VTop: VAlignValue<3>};
  HAlign: {HLeft: HAlignValue<0>, HCenter: HAlignValue<1>, HRight: HAlignValue<2>, HAligned: HAlignValue<3>, HMiddle: HAlignValue<4>, HFit: HAlignValue<5>};
  DRW_Text: {
    new(): DRW_Text;
  };
  Attach: {TopLeft: AttachValue<1>, TopCenter: AttachValue<2>, TopRight: AttachValue<3>, MiddleLeft: AttachValue<4>, MiddleCenter: AttachValue<5>, MiddleRight: AttachValue<6>, BottomLeft: AttachValue<7>, BottomCenter: AttachValue<8>, BottomRight: AttachValue<9>};
  DRW_MText: {
    new(): DRW_MText;
  };
  DRW_Vertex: {
    new(): DRW_Vertex;
    new(_0: number, _1: number, _2: number, _3: number): DRW_Vertex;
  };
  DRW_Polyline: {
    new(): DRW_Polyline;
  };
  DRW_Spline: {
    new(): DRW_Spline;
  };
  DRW_HatchLoop: {
    new(_0: number): DRW_HatchLoop;
  };
  DRW_Hatch: {
    new(): DRW_Hatch;
  };
  DRW_Image: {
    new(): DRW_Image;
  };
  DRW_Dimension: {
    new(): DRW_Dimension;
  };
  DRW_DimAligned: {
    new(): DRW_DimAligned;
    new(_0: DRW_Dimension): DRW_DimAligned;
  };
  DRW_DimLinear: {
    new(): DRW_DimLinear;
    new(_0: DRW_Dimension): DRW_DimLinear;
  };
  DRW_DimRadial: {
    new(): DRW_DimRadial;
    new(_0: DRW_Dimension): DRW_DimRadial;
  };
  DRW_DimDiametric: {
    new(): DRW_DimDiametric;
    new(_0: DRW_Dimension): DRW_DimDiametric;
  };
  DRW_DimAngular: {
    new(): DRW_DimAngular;
    new(_0: DRW_Dimension): DRW_DimAngular;
  };
  DRW_DimAngular3p: {
    new(): DRW_DimAngular3p;
    new(_0: DRW_Dimension): DRW_DimAngular3p;
  };
  DRW_DimOrdinate: {
    new(): DRW_DimOrdinate;
    new(_0: DRW_Dimension): DRW_DimOrdinate;
  };
  DRW_Leader: {
    new(): DRW_Leader;
  };
  DRW_Viewport: {
    new(): DRW_Viewport;
  };
  DRW_Interface: {};
  dxfRW: {
    new(_0: EmbindString): dxfRW;
  };
  dwgR: {
    new(_0: EmbindString): dwgR;
  };
  DRW_EntityList: {
    new(): DRW_EntityList;
  };
  DRW_LTypeList: {
    new(): DRW_LTypeList;
  };
  DRW_LayerList: {
    new(): DRW_LayerList;
  };
  DRW_DimstyleList: {
    new(): DRW_DimstyleList;
  };
  DRW_VportList: {
    new(): DRW_VportList;
  };
  DRW_TextstyleList: {
    new(): DRW_TextstyleList;
  };
  DRW_AppIdList: {
    new(): DRW_AppIdList;
  };
  DRW_ifaceBlockList: {
    new(): DRW_ifaceBlockList;
  };
  DRW_ifaceImgList: {
    new(): DRW_ifaceImgList;
  };
  dx_ifaceImg: {
    new(): dx_ifaceImg;
    new(_0: DRW_Image): dx_ifaceImg;
  };
  dx_ifaceBlock: {
    new(): dx_ifaceBlock;
    new(_0: DRW_Block): dx_ifaceBlock;
  };
  dx_data: {
    new(): dx_data;
  };
  dx_iface: {
    new(): dx_iface;
  };
}

export type MainModule = WasmModule & typeof RuntimeExports & EmbindModule;
