#include "EnergyProfiler.h"

#if ENERGY_PROFILER_ENABLED

struct EnergyProfileSnapshot {
    uint32_t runtimeMs;
    uint32_t loopIterations;
    uint32_t encoderEvents;
    uint32_t imuPolls;
    uint32_t bleReports;
    uint64_t busyTimeUs;
    uint64_t requestedSleepUs;

    float peakLoopRate;
    float peakEncoderRate;
    float peakImuPollRate;
    float peakBleReportRate;
    float peakBusyPercent;
    float lowestSleepOpportunityPercent;
};

static volatile uint32_t encoderEvents = 0;
static volatile bool captureEnabled = false;

static uint32_t loopIterations = 0;
static uint32_t imuPolls = 0;
static uint32_t bleReports = 0;
static uint64_t busyTimeUs = 0;
static uint64_t requestedSleepUs = 0;
static uint32_t loopStartedUs = 0;
static uint32_t sessionStartedMs = 0;

// Peak values are calculated over approximately one-second windows. This
// avoids meaningless instantaneous peaks from individual loop iterations.
static uint32_t windowStartedMs = 0;
static uint32_t windowLoopStart = 0;
static uint32_t windowEncoderStart = 0;
static uint32_t windowImuStart = 0;
static uint32_t windowBleStart = 0;
static uint64_t windowBusyStartUs = 0;
static uint64_t windowSleepStartUs = 0;
static float peakLoopRate = 0.0f;
static float peakEncoderRate = 0.0f;
static float peakImuPollRate = 0.0f;
static float peakBleReportRate = 0.0f;
static float peakBusyPercent = 0.0f;
static float lowestSleepOpportunityPercent = 100.0f;
static bool peakWindowRecorded = false;

static EnergyProfileSnapshot pendingSnapshot = {};
static bool snapshotPending = false;

static uint32_t readEncoderEventsAtomic()
{
    noInterrupts();
    const uint32_t value = encoderEvents;
    interrupts();
    return value;
}

static void resetPeakWindow(uint32_t nowMs)
{
    windowStartedMs = nowMs;
    windowLoopStart = loopIterations;
    windowEncoderStart = readEncoderEventsAtomic();
    windowImuStart = imuPolls;
    windowBleStart = bleReports;
    windowBusyStartUs = busyTimeUs;
    windowSleepStartUs = requestedSleepUs;
}

static void samplePeakWindow(bool includePartialWindow)
{
    if (!captureEnabled && !includePartialWindow) return;

    const uint32_t nowMs = millis();
    const uint32_t elapsedMs = nowMs - windowStartedMs;

    if (elapsedMs == 0) return;
    if (!includePartialWindow && elapsedMs < 1000UL) return;

    const uint32_t currentEncoderEvents = readEncoderEventsAtomic();
    const float scale = 1000.0f / (float)elapsedMs;
    const float loopRate = (loopIterations - windowLoopStart) * scale;
    const float encoderRate = (currentEncoderEvents - windowEncoderStart) * scale;
    const float imuRate = (imuPolls - windowImuStart) * scale;
    const float bleRate = (bleReports - windowBleStart) * scale;
    const uint64_t elapsedUs = (uint64_t)elapsedMs * 1000ULL;
    const float busyPercent = 100.0f *
        (float)(busyTimeUs - windowBusyStartUs) / (float)elapsedUs;
    const float sleepOpportunityPercent = 100.0f *
        (float)(requestedSleepUs - windowSleepStartUs) / (float)elapsedUs;

    if (loopRate > peakLoopRate) peakLoopRate = loopRate;
    if (encoderRate > peakEncoderRate) peakEncoderRate = encoderRate;
    if (imuRate > peakImuPollRate) peakImuPollRate = imuRate;
    if (bleRate > peakBleReportRate) peakBleReportRate = bleRate;
    if (busyPercent > peakBusyPercent) peakBusyPercent = busyPercent;
    if (!peakWindowRecorded || sleepOpportunityPercent < lowestSleepOpportunityPercent) {
        lowestSleepOpportunityPercent = sleepOpportunityPercent;
    }
    peakWindowRecorded = true;

    resetPeakWindow(nowMs);
}

static void resetSession()
{
    noInterrupts();
    encoderEvents = 0;
    interrupts();

    loopIterations = 0;
    imuPolls = 0;
    bleReports = 0;
    busyTimeUs = 0;
    requestedSleepUs = 0;
    peakLoopRate = 0.0f;
    peakEncoderRate = 0.0f;
    peakImuPollRate = 0.0f;
    peakBleReportRate = 0.0f;
    peakBusyPercent = 0.0f;
    lowestSleepOpportunityPercent = 100.0f;
    peakWindowRecorded = false;

    sessionStartedMs = millis();
    resetPeakWindow(sessionStartedMs);
}

static void freezeSession()
{
    // Include the final partial second so a short burst immediately before
    // connecting USB is not omitted from the peak statistics.
    samplePeakWindow(true);
    captureEnabled = false;

    pendingSnapshot.encoderEvents = readEncoderEventsAtomic();
    pendingSnapshot.runtimeMs = millis() - sessionStartedMs;
    pendingSnapshot.loopIterations = loopIterations;
    pendingSnapshot.imuPolls = imuPolls;
    pendingSnapshot.bleReports = bleReports;
    pendingSnapshot.busyTimeUs = busyTimeUs;
    pendingSnapshot.requestedSleepUs = requestedSleepUs;
    pendingSnapshot.peakLoopRate = peakLoopRate;
    pendingSnapshot.peakEncoderRate = peakEncoderRate;
    pendingSnapshot.peakImuPollRate = peakImuPollRate;
    pendingSnapshot.peakBleReportRate = peakBleReportRate;
    pendingSnapshot.peakBusyPercent = peakBusyPercent;
    pendingSnapshot.lowestSleepOpportunityPercent = peakWindowRecorded
        ? lowestSleepOpportunityPercent
        : 0.0f;
    snapshotPending = true;
}

static void printDuration(uint32_t runtimeMs)
{
    const uint32_t totalSeconds = runtimeMs / 1000UL;
    const uint32_t hours = totalSeconds / 3600UL;
    const uint32_t minutes = (totalSeconds % 3600UL) / 60UL;
    const uint32_t seconds = totalSeconds % 60UL;

    if (hours > 0) {
        Serial.print(hours);
        Serial.print(" h ");
    }
    if (minutes > 0 || hours > 0) {
        Serial.print(minutes);
        Serial.print(" min ");
    }
    Serial.print(seconds);
    Serial.println(" s");
}

static void printRateLine(const char* averageLabel, const char* peakLabel,
                          uint32_t total, float seconds, float peak)
{
    Serial.print(averageLabel);
    Serial.println(seconds > 0.0f ? (float)total / seconds : 0.0f, 1);
    Serial.print(peakLabel);
    Serial.println(peak, 1);
}

static void printSnapshot()
{
    const float seconds = pendingSnapshot.runtimeMs / 1000.0f;
    const float busyPercent = pendingSnapshot.runtimeMs > 0
        ? 100.0f * (float)pendingSnapshot.busyTimeUs /
            ((float)pendingSnapshot.runtimeMs * 1000.0f)
        : 0.0f;
    const float sleepOpportunityPercent = pendingSnapshot.runtimeMs > 0
        ? 100.0f * (float)pendingSnapshot.requestedSleepUs /
            ((float)pendingSnapshot.runtimeMs * 1000.0f)
        : 0.0f;

    Serial.println();
    Serial.println("========================================");
    Serial.println(" GM-6000 BLE ENERGY PROFILE");
    Serial.println(" Captured before USB was connected");
    Serial.println(" Peaks use approximately 1-second windows");
    Serial.println("========================================");
    Serial.print("BLE test duration:       ");
    printDuration(pendingSnapshot.runtimeMs);

    Serial.print("Loop iterations:         ");
    Serial.println(pendingSnapshot.loopIterations);
    printRateLine("Average loops/s:         ", "Peak loops/s:            ",
                  pendingSnapshot.loopIterations, seconds,
                  pendingSnapshot.peakLoopRate);

    Serial.print("Encoder steps:           ");
    Serial.println(pendingSnapshot.encoderEvents);
    printRateLine("Average encoder steps/s: ", "Peak encoder steps/s:    ",
                  pendingSnapshot.encoderEvents, seconds,
                  pendingSnapshot.peakEncoderRate);

    Serial.print("IMU polls:               ");
    Serial.println(pendingSnapshot.imuPolls);
    printRateLine("Average IMU polls/s:     ", "Peak IMU polls/s:        ",
                  pendingSnapshot.imuPolls, seconds,
                  pendingSnapshot.peakImuPollRate);

    Serial.print("BLE HID reports:         ");
    Serial.println(pendingSnapshot.bleReports);
    printRateLine("Average BLE reports/s:   ", "Peak BLE reports/s:      ",
                  pendingSnapshot.bleReports, seconds,
                  pendingSnapshot.peakBleReportRate);

    Serial.print("Average main-task busy:  ");
    Serial.print(busyPercent, 2);
    Serial.println(" % (measured main loop)");
    Serial.print("Peak main-task busy:     ");
    Serial.print(pendingSnapshot.peakBusyPercent, 2);
    Serial.println(" % (1-second window)");
    Serial.print("Average sleep chance:    ");
    Serial.print(sleepOpportunityPercent, 2);
    Serial.println(" % (requested delay estimate)");
    Serial.print("Lowest sleep chance:     ");
    Serial.print(pendingSnapshot.lowestSleepOpportunityPercent, 2);
    Serial.println(" % (1-second window)");
    Serial.println("========================================");
    Serial.println("Disconnect USB to start a new BLE test.");
    Serial.println();
}

void energyProfilerBegin(bool usbMode)
{
    resetSession();
    captureEnabled = !usbMode;
    snapshotPending = false;
}

void energyProfilerLoopBegin()
{
    if (!captureEnabled) return;

    loopIterations++;
    loopStartedUs = micros();
}

void energyProfilerBeforeDelay(uint32_t requestedDelayMs)
{
    if (!captureEnabled) return;

    const uint32_t nowUs = micros();
    busyTimeUs += (uint32_t)(nowUs - loopStartedUs);
    requestedSleepUs += (uint64_t)requestedDelayMs * 1000ULL;
}

void energyProfilerEncoderEventFromIsr()
{
    if (captureEnabled) {
        encoderEvents++;
    }
}

void energyProfilerImuPoll()
{
    if (captureEnabled) {
        imuPolls++;
    }
}

void energyProfilerBleReport()
{
    if (captureEnabled) {
        bleReports++;
    }
}

void energyProfilerUsbReport()
{
    // USB reports are intentionally excluded. This profiler captures the
    // BLE session that took place before the USB host was connected.
}

void energyProfilerUsbModeChanged(bool usbMode)
{
    if (usbMode) {
        if (captureEnabled) {
            freezeSession();
        }
    } else {
        snapshotPending = false;
        resetSession();
        captureEnabled = true;
    }
}

void energyProfilerUpdate()
{
    if (captureEnabled) {
        samplePeakWindow(false);
    }

    // Keep the snapshot in RAM indefinitely until a serial terminal has
    // actually opened. USB may be connected before PlatformIO Serial Monitor
    // is started; no report is lost in that situation.
    if (!snapshotPending || !Serial) return;

    printSnapshot();
    snapshotPending = false;
}

#endif
