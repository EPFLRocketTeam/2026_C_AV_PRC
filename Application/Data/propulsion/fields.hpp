
#ifndef X_PRC_SENSORS_STORE_ENGINE
#define X_PRC_SENSORS_STORE_ENGINE(...) \
  X_FIELD (double, pressure_C, ##__VA_ARGS__) \
  X_FIELD (double, pressure_OIN, ##__VA_ARGS__) \
  X_FIELD (double, pressure_EIN, ##__VA_ARGS__) \
  X_FIELD (double, temperature_C, ##__VA_ARGS__) \
  X_FIELD (double, temperatur_OIN, ##__VA_ARGS__) \
  X_FIELD (double, temperatur_EIN, ##__VA_ARGS__)
#endif

#ifndef X_PRC_SENSORS_STORE_LOX
#define X_PRC_SENSORS_STORE_LOX(...) \
  X_raw_FIELD(double, pressure_OTA1, ##__VA_ARGS__) \
  X_raw_FIELD(double, pressure_OTA2, ##__VA_ARGS__) \
  X_raw_FIELD(double, pressure_OTA3, ##__VA_ARGS__) \
  X_proc_FIELD(double, pressure_OTA_mean, ##__VA_ARGS__) \
  X_FIELD(double, pressure_HPO, ##__VA_ARGS__) \
  X_FIELD(double, temperature_OTA1, ##__VA_ARGS__) \
  X_FIELD(double, temperature_OTA2, ##__VA_ARGS__) \
  X_FIELD(double, temperature_OTA3, ##__VA_ARGS__) \
  X_FIELD(double, temperature_OTA4, ##__VA_ARGS__) \
  X_raw_FIELD(double, FLS, ##__VA_ARGS__)
#endif

#ifndef X_PRC_SENSORS_STORE_ETH
#define X_PRC_SENSORS_STORE_ETH(...) \
  X_raw_FIELD(double, pressure_ETA1, ##__VA_ARGS__) \
  X_raw_FIELD(double, pressure_ETA2, ##__VA_ARGS__) \
  X_raw_FIELD(double, pressure_ETA3, ##__VA_ARGS__) \
  X_proc_FIELD(double, pressure_ETA_mean, ##__VA_ARGS__) \
  X_FIELD(double, pressure_HPE, ##__VA_ARGS__)
#endif
