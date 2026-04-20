import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (CONF_VERSION, ICON_BUG, ENTITY_CATEGORY_DIAGNOSTIC)
from . import CONF_PENTAIR_IF_IC_ID, PentairIfIcComponent

DEPENDENCIES = ["pentair_if_ic"]

CONF_PROGRAM = "program"
CONF_VERSION = "version"
CONF_DEBUG   = "debug"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_PENTAIR_IF_IC_ID): cv.use_id(PentairIfIcComponent),
    cv.Optional(CONF_PROGRAM): text_sensor.text_sensor_schema(),
    cv.Optional(CONF_VERSION): text_sensor.text_sensor_schema(icon=ICON_BUG, entity_category=ENTITY_CATEGORY_DIAGNOSTIC),
    cv.Optional(CONF_DEBUG):   text_sensor.text_sensor_schema(icon=ICON_BUG, entity_category=ENTITY_CATEGORY_DIAGNOSTIC),
})

async def to_code(config):
    var = await cg.get_variable(config[CONF_PENTAIR_IF_IC_ID])
    if c := config.get(CONF_PROGRAM): cg.add(var.set_if_program(await text_sensor.new_text_sensor(c)))
    if c := config.get(CONF_VERSION): cg.add(var.set_ic_version_text_sensor(await text_sensor.new_text_sensor(c)))
    if c := config.get(CONF_DEBUG):   cg.add(var.set_ic_debug_text_sensor(await text_sensor.new_text_sensor(c)))
