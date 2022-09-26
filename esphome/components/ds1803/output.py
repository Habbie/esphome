import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_CHANNEL, CONF_ID
from . import DS1803Output, ds1803_ns

DEPENDENCIES = ["ds1803"]

DS1803Channel = ds1803_ns.class_("DS1803Channel", output.FloatOutput)
CONF_DS1803_ID = "ds1803_id"

CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.Required(CONF_ID): cv.declare_id(DS1803Channel),
        cv.GenerateID(CONF_DS1803_ID): cv.use_id(DS1803Output),
        cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=1),
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_DS1803_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_channel(config[CONF_CHANNEL]))
    cg.add(paren.register_channel(var))
    await output.register_output(var, config)
    return var
