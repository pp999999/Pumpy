import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    DEVICE_CLASS_RUNNING, DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_DIAGNOSTIC, ICON_BUG,
)
from . import CONF_PENTAIR_IF_IC_ID, PentairIfIcComponent

DEPENDENCIES = ["pentair_if_ic"]

CONF_RUNNING = "running"
CONF_NO_FLOW = "no_flow"
CONF_LOW_SALT = "low_salt"
CONF_HIGH_SALT = "high_salt"
CONF_CLEAN = "clean"
CONF_HIGH_CURRENT = "high_current"
CONF_LOW_VOLTS = "low_volts"
CONF_LOW_TEMP = "low_temp"
CONF_CHECK_PCB = "check_pcb"

_DIAG = dict(device_class=DEVICE_CLASS_PROBLEM, entity_category=ENTITY_CATEGORY_DIAGNOSTIC, icon=ICON_BUG)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_PENTAIR_IF_IC_ID): cv.use_id(PentairIfIcComponent),
    cv.Optional(CONF_RUNNING):      binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_RUNNING),
    cv.Optional(CONF_NO_FLOW):      binary_sensor.binary_sensor_schema(**_DIAG),
    cv.Optional(CONF_LOW_SALT):     binary_sensor.binary_sensor_schema(**_DIAG),
    cv.Optional(CONF_HIGH_SALT):    binary_sensor.binary_sensor_schema(**_DIAG),
    cv.Optional(CONF_CLEAN):        binary_sensor.binary_sensor_schema(**_DIAG),
    cv.Optional(CONF_HIGH_CURRENT): binary_sensor.binary_sensor_schema(**_DIAG),
    cv.Optional(CONF_LOW_VOLTS):    binary_sensor.binary_sensor_schema(**_DIAG),
    cv.Optional(CONF_LOW_TEMP):     binary_sensor.binary_sensor_schema(**_DIAG),
    cv.Optional(CONF_CHECK_PCB):    binary_sensor.binary_sensor_schema(**_DIAG),
})

async def to_code(config):
    var = await cg.get_variable(config[CONF_PENTAIR_IF_IC_ID])
    if c := config.get(CONF_RUNNING):      cg.add(var.set_if_running(await binary_sensor.new_binary_sensor(c)))
    if c := config.get(CONF_NO_FLOW):      cg.add(var.set_no_flow_binary_sensor(await binary_sensor.new_binary_sensor(c)))
    if c := config.get(CONF_LOW_SALT):     cg.add(var.set_low_salt_binary_sensor(await binary_sensor.new_binary_sensor(c)))
    if c := config.get(CONF_HIGH_SALT):    cg.add(var.set_high_salt_binary_sensor(await binary_sensor.new_binary_sensor(c)))
    if c := config.get(CONF_CLEAN):        cg.add(var.set_clean_binary_sensor(await binary_sensor.new_binary_sensor(c)))
    if c := config.get(CONF_HIGH_CURRENT): cg.add(var.set_high_current_binary_sensor(await binary_sensor.new_binary_sensor(c)))
    if c := config.get(CONF_LOW_VOLTS):    cg.add(var.set_low_volts_binary_sensor(await binary_sensor.new_binary_sensor(c)))
    if c := config.get(CONF_LOW_TEMP):     cg.add(var.set_low_temp_binary_sensor(await binary_sensor.new_binary_sensor(c)))
    if c := config.get(CONF_CHECK_PCB):    cg.add(var.set_check_pcb_binary_sensor(await binary_sensor.new_binary_sensor(c)))
