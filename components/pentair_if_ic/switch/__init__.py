import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import (DEVICE_CLASS_SWITCH, ENTITY_CATEGORY_CONFIG, ICON_PULSE)
from .. import CONF_PENTAIR_IF_IC_ID, PentairIfIcComponent, pentair_if_ic_ns

TakeoverModeSwitch = pentair_if_ic_ns.class_("TakeoverModeSwitch", switch.Switch)
PumpRunSwitch      = pentair_if_ic_ns.class_("PumpRunSwitch", switch.Switch)

CONF_TAKEOVER_MODE = "takeover_mode"
CONF_PUMP_RUN      = "pump_run"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_PENTAIR_IF_IC_ID): cv.use_id(PentairIfIcComponent),
    cv.Optional(CONF_TAKEOVER_MODE): switch.switch_schema(
        TakeoverModeSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_PULSE,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    cv.Optional(CONF_PUMP_RUN): switch.switch_schema(
        PumpRunSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:pump",
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
})

async def to_code(config):
    pentair_component = await cg.get_variable(config[CONF_PENTAIR_IF_IC_ID])
    if c := config.get(CONF_TAKEOVER_MODE):
        s = await switch.new_switch(c)
        await cg.register_parented(s, config[CONF_PENTAIR_IF_IC_ID])
        cg.add(pentair_component.set_takeover_mode_switch(s))
    if c := config.get(CONF_PUMP_RUN):
        s = await switch.new_switch(c)
        await cg.register_parented(s, config[CONF_PENTAIR_IF_IC_ID])
        cg.add(pentair_component.set_pump_run_switch(s))
