import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_POWER, DEVICE_CLASS_VOLUME_FLOW_RATE, DEVICE_CLASS_PRESSURE,
    DEVICE_CLASS_DURATION, DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLATILE_ORGANIC_COMPOUNDS_PARTS,
    UNIT_WATT, UNIT_REVOLUTIONS_PER_MINUTE, UNIT_CUBIC_METER_PER_HOUR,
    UNIT_MINUTE, UNIT_PARTS_PER_MILLION, UNIT_PERCENT,
)
from . import CONF_PENTAIR_IF_IC_ID, PentairIfIcComponent

DEPENDENCIES = ["pentair_if_ic"]

CONF_POWER = "power"
CONF_RPM = "rpm"
CONF_FLOW = "flow"
CONF_PRESSURE = "pressure"
CONF_TIME_REMAINING = "time_remaining"
CONF_CLOCK = "clock"
CONF_SALT_PPM = "salt_ppm"
CONF_WATER_TEMP = "water_temp"
CONF_STATUS = "status"
CONF_ERROR = "error"
CONF_SET_PERCENT = "set_percent"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_PENTAIR_IF_IC_ID): cv.use_id(PentairIfIcComponent),
    cv.Optional(CONF_POWER):          sensor.sensor_schema(unit_of_measurement=UNIT_WATT, accuracy_decimals=0, device_class=DEVICE_CLASS_POWER),
    cv.Optional(CONF_RPM):            sensor.sensor_schema(unit_of_measurement=UNIT_REVOLUTIONS_PER_MINUTE, accuracy_decimals=0),
    cv.Optional(CONF_FLOW):           sensor.sensor_schema(unit_of_measurement=UNIT_CUBIC_METER_PER_HOUR, accuracy_decimals=2, device_class=DEVICE_CLASS_VOLUME_FLOW_RATE),
    cv.Optional(CONF_PRESSURE):       sensor.sensor_schema(unit_of_measurement="bar", accuracy_decimals=3, device_class=DEVICE_CLASS_PRESSURE),
    cv.Optional(CONF_TIME_REMAINING): sensor.sensor_schema(unit_of_measurement=UNIT_MINUTE, accuracy_decimals=0, device_class=DEVICE_CLASS_DURATION),
    cv.Optional(CONF_CLOCK):          sensor.sensor_schema(unit_of_measurement=UNIT_MINUTE, accuracy_decimals=0, device_class=DEVICE_CLASS_DURATION),
    cv.Optional(CONF_SALT_PPM):       sensor.sensor_schema(device_class=DEVICE_CLASS_VOLATILE_ORGANIC_COMPOUNDS_PARTS, unit_of_measurement=UNIT_PARTS_PER_MILLION),
    cv.Optional(CONF_WATER_TEMP):     sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement="°F"),
    cv.Optional(CONF_STATUS):         sensor.sensor_schema(),
    cv.Optional(CONF_ERROR):          sensor.sensor_schema(),
    cv.Optional(CONF_SET_PERCENT):    sensor.sensor_schema(unit_of_measurement=UNIT_PERCENT),
})

async def to_code(config):
    var = await cg.get_variable(config[CONF_PENTAIR_IF_IC_ID])
    if c := config.get(CONF_POWER):          cg.add(var.set_if_power(await sensor.new_sensor(c)))
    if c := config.get(CONF_RPM):            cg.add(var.set_if_rpm(await sensor.new_sensor(c)))
    if c := config.get(CONF_FLOW):           cg.add(var.set_if_flow(await sensor.new_sensor(c)))
    if c := config.get(CONF_PRESSURE):       cg.add(var.set_if_pressure(await sensor.new_sensor(c)))
    if c := config.get(CONF_TIME_REMAINING): cg.add(var.set_if_time_remaining(await sensor.new_sensor(c)))
    if c := config.get(CONF_CLOCK):          cg.add(var.set_if_clock(await sensor.new_sensor(c)))
    if c := config.get(CONF_SALT_PPM):       cg.add(var.set_salt_ppm_sensor(await sensor.new_sensor(c)))
    if c := config.get(CONF_WATER_TEMP):     cg.add(var.set_water_temp_sensor(await sensor.new_sensor(c)))
    if c := config.get(CONF_STATUS):         cg.add(var.set_ic_status_sensor(await sensor.new_sensor(c)))
    if c := config.get(CONF_ERROR):          cg.add(var.set_ic_error_sensor(await sensor.new_sensor(c)))
    if c := config.get(CONF_SET_PERCENT):    cg.add(var.set_set_percent_sensor(await sensor.new_sensor(c)))
