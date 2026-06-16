$root = Split-Path -Parent $PSScriptRoot
$displayHeader = Get-Content -Raw (Join-Path $root 'BiasPro\DisplayManager.h')
$controllerHeader = Get-Content -Raw (Join-Path $root 'BiasPro\ApplicationController.h')
$controller = Get-Content -Raw (Join-Path $root 'BiasPro\ApplicationController.cpp')
$protectionHeader = Get-Content -Raw (Join-Path $root 'BiasPro\ProtectionSystem.h')

$requiredDisplayApis = @(
  'updateTubeSelection',
  'drawLiveBiasFrame',
  'updateLiveBiasValues',
  'updateSensorTelemetryValues',
  'updateProfileManagerSelection',
  'updateCalibrationValues'
)

foreach ($api in $requiredDisplayApis) {
  if ($displayHeader -notmatch $api) {
    throw "Missing DisplayManager incremental API: $api"
  }
}

$requiredControllerCalls = @(
  'display_.updateTubeSelection',
  'display_.drawLiveBiasFrame',
  'display_.updateLiveBiasValues',
  'display_.updateSensorTelemetryValues',
  'display_.updateProfileManagerSelection',
  'display_.updateCalibrationValues'
)

foreach ($call in $requiredControllerCalls) {
  if ($controller -notmatch [regex]::Escape($call)) {
    throw "ApplicationController does not use incremental display call: $call"
  }
}

$liveBranchStart = $controller.IndexOf('if (activeScreen_ == ScreenId::LiveBias)')
$liveBranchEnd = $controller.IndexOf('if (activeScreen_ == ScreenId::FaultLockout)')
if ($liveBranchStart -lt 0 -or $liveBranchEnd -lt $liveBranchStart) {
  throw 'Could not locate LiveBias controller branch'
}

$liveBranch = $controller.Substring($liveBranchStart, $liveBranchEnd - $liveBranchStart)
$drawFrameIndex = $liveBranch.IndexOf('display_.drawLiveBiasFrame')
$readAdcIndex = $liveBranch.IndexOf('hardware_.readAdcFrame')
if ($drawFrameIndex -lt 0 -or $readAdcIndex -lt 0 -or $drawFrameIndex -gt $readAdcIndex) {
  throw 'LiveBias frame must be drawn before ADC sampling so the previous screen does not linger'
}

$tubeSelectStart = $controller.IndexOf('if (activeScreen_ == ScreenId::TubeSelect)')
$tubeSelectEnd = $controller.IndexOf('if (activeScreen_ == ScreenId::LiveBias)')
if ($tubeSelectStart -lt 0 -or $tubeSelectEnd -lt $tubeSelectStart) {
  throw 'Could not locate TubeSelect controller branch'
}

$tubeSelectBranch = $controller.Substring($tubeSelectStart, $tubeSelectEnd - $tubeSelectStart)
if ($tubeSelectBranch -notmatch 'activeScreen_ = ScreenId::LiveBias;[\s\S]{0,180}return;') {
  throw 'TubeSelect must return immediately after switching to LiveBias'
}

if ($tubeSelectBranch -notmatch 'activeScreen_ = ScreenId::ProfileManager;[\s\S]{0,180}return;') {
  throw 'TubeSelect must return immediately after switching to ProfileManager'
}

$displaySource = Get-Content -Raw (Join-Path $root 'BiasPro\DisplayManager.cpp')
$profileSource = Get-Content -Raw (Join-Path $root 'BiasPro\ProfileEditor.cpp')
$profileHeader = Get-Content -Raw (Join-Path $root 'BiasPro\ProfileEditor.h')
$liveUpdateStart = $displaySource.IndexOf('void DisplayManager::updateLiveBiasValues')
$telemetryFrameStart = $displaySource.IndexOf('void DisplayManager::drawSensorTelemetryFrame')
if ($liveUpdateStart -lt 0 -or $telemetryFrameStart -lt $liveUpdateStart) {
  throw 'Could not locate updateLiveBiasValues body'
}

$liveUpdateBody = $displaySource.Substring($liveUpdateStart, $telemetryFrameStart - $liveUpdateStart)
if ($liveUpdateBody -match 'fillRect') {
  throw 'Live value refresh must not blank value rectangles every sample'
}

if ($displayHeader -notmatch 'liveValuesPrimed_') {
  throw 'DisplayManager must cache live values to avoid rewriting unchanged readings'
}

$calDrawStart = $displaySource.IndexOf('void DisplayManager::drawCalibration')
$calUpdateStart = $displaySource.IndexOf('void DisplayManager::updateCalibrationValues')
$lockoutStart = $displaySource.IndexOf('void DisplayManager::drawVoltageLockout')
if ($calDrawStart -lt 0 -or $calUpdateStart -lt $calDrawStart -or $lockoutStart -lt $calUpdateStart) {
  throw 'Could not locate calibration display methods'
}

$calDrawBody = $displaySource.Substring($calDrawStart, $calUpdateStart - $calDrawStart)
$calUpdateBody = $displaySource.Substring($calUpdateStart, $lockoutStart - $calUpdateStart)
if ($calUpdateBody -match 'fillScreen|fillRect\s*\(\s*8\s*,\s*24\s*,\s*145\s*,\s*82') {
  throw 'Calibration incremental update must not clear the screen or full value block'
}

if ($displayHeader -notmatch 'drawCalibrationRow' -or $calDrawBody -notmatch 'drawCalibrationRow' -or $calUpdateBody -notmatch 'drawCalibrationRow') {
  throw 'Calibration full and incremental paths must share drawCalibrationRow'
}

if ($displayHeader -notmatch 'calibrationValuesPrimed_') {
  throw 'DisplayManager must cache calibration values for incremental redraw'
}

$editStart = $profileSource.IndexOf('void ProfileEditor::drawEditScreen')
$deleteStart = $profileSource.IndexOf('void ProfileEditor::drawDeleteConfirm')
if ($editStart -lt 0 -or $deleteStart -lt $editStart) {
  throw 'Could not locate ProfileEditor edit screen body'
}

$editBody = $profileSource.Substring($editStart, $deleteStart - $editStart)
if ($editBody -match 'fillRect\s*\(\s*7\s*,\s*26\s*,\s*146\s*,\s*76') {
  throw 'Profile edit incremental update must not clear the full edit body'
}

if ($profileHeader -notmatch 'drawEditRow' -or $editBody -notmatch 'drawEditRow') {
  throw 'Profile editor full and incremental paths must share drawEditRow'
}

if ($profileHeader -notmatch 'editorValuesPrimed_') {
  throw 'ProfileEditor must cache edit values for incremental redraw'
}

if ($displayHeader -notmatch 'drawHardwareFault') {
  throw 'DisplayManager must expose a hardware fault screen'
}

if ($protectionHeader -notmatch 'bool\s+isLocked\s*\(\s*\)\s+const') {
  throw 'ProtectionSystem must expose isLocked() const'
}

if ($controllerHeader -notmatch 'bool\s+hardwareReady_') {
  throw 'ApplicationController must store hardwareReady_'
}

if ($controller -notmatch 'hardwareReady_\s*=\s*hardware_\.begin\(\)') {
  throw 'ApplicationController must assign hardwareReady_ from HardwareIO::begin()'
}

$hardwareFaultIndex = $controller.IndexOf('if (!hardwareReady_)')
$buttonReadIndex = $controller.IndexOf('hardware_.readButtonEvent()')
$firstAdcReadIndex = $controller.IndexOf('hardware_.readAdcFrame')
if ($hardwareFaultIndex -lt 0) {
  throw 'ApplicationController must guard tick() with a hardware fault branch'
}

if ($buttonReadIndex -lt 0 -or $hardwareFaultIndex -gt $buttonReadIndex) {
  throw 'Hardware fault branch must run before button polling'
}

if ($firstAdcReadIndex -lt 0 -or $hardwareFaultIndex -gt $firstAdcReadIndex) {
  throw 'Hardware fault branch must run before ADC polling'
}

$hardwareFaultBranch = $controller.Substring($hardwareFaultIndex, [Math]::Min(260, $controller.Length - $hardwareFaultIndex))
if ($hardwareFaultBranch -notmatch 'display_\.drawHardwareFault\(\)' -or $hardwareFaultBranch -notmatch 'return;') {
  throw 'Hardware fault branch must draw drawHardwareFault() and return'
}

$telemetryStart = $controller.IndexOf('if (activeScreen_ == ScreenId::SensorTelemetry)')
$profileManagerStart = $controller.IndexOf('if (activeScreen_ == ScreenId::ProfileManager)')
if ($telemetryStart -lt 0 -or $profileManagerStart -lt $telemetryStart) {
  throw 'Could not locate SensorTelemetry controller branch'
}

$telemetryBranch = $controller.Substring($telemetryStart, $profileManagerStart - $telemetryStart)
if ($telemetryBranch -notmatch 'protection_\.isLocked\(\)' -or $telemetryBranch -notmatch 'activeScreen_\s*=\s*ScreenId::FaultLockout') {
  throw 'SensorTelemetry exit must return to FaultLockout when protection remains locked'
}

'screen redraw static check passed'
