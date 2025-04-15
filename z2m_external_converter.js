const m = require('zigbee-herdsman-converters/lib/modernExtend');
const Zcl = require('zigbee-herdsman').Zcl;
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const e = exposes.presets;
const ea = exposes.access;

const definition = {
  zigbeeModel: ['MoniBee'],
  model: 'MoniBee',
  vendor: 'Echolot',
  description: 'Controls HDMI Displays over DDC',
  extend: [
    m.deviceEndpoints({
      endpoints: {
        'internal_temperature': 10,
        'veml_7700': 11,
        'ddc': 12,
        'ir': 13,
      }
    }),
    m.temperature({
      endpointNames: ['internal_temperature'],
      label: 'Internal Temperature',
      reporting: {min: 5, max: 60, change: 100},
    }),
    m.illuminance({
      endpointNames: ['veml_7700'],
      label: 'Ambient Illuminance',
      name: 'ambient_illuminance',
      precision: 0,
      reporting: {min: 1, max: 5, change: 10},
    }),
    m.deviceAddCustomCluster('manuSpecificMoniBeeDDC', {
      ID: 0xFF00,
      attributes: {
        ddc_input: {ID: 0x0000, type: Zcl.DataType.ENUM16},
        ddc_brightness: {ID: 0x0001, type: Zcl.DataType.UINT16},
        ddc_power_mode: {ID: 0x0002, type: Zcl.DataType.ENUM8},
      },
      commands: {},
      commandsResponse: {},
    }),
    m.deviceAddCustomCluster('manuSpecificMoniBeeIR', {
      ID: 0xFF01,
      attributes: {},
      commands: {
        sendIRCommand: {
          ID: 0x0000, 
          parameters: [
            {name: 'address', type: Zcl.DataType.UINT16},
            {name: 'command', type: Zcl.DataType.UINT16},
          ],
        },
      },
      commandsResponse: {},
    }),
    m.enumLookup({
      name: 'ddc_input',
      label: 'Display Input',
      description: 'Currently active display input',
      lookup: {
        'Unknown 1': 0x00,
        'Unknown 2': 0x01,
        'Unknown 3': 0x03,
        'HDMI 1': 0x11,
        'HDMI 2': 0x12,
        'HDMI 3': 0x13,
        'HDMI 4': 0x14,
        'DisplayPort 1': 0x0F,
        'DisplayPort 2': 0x10,
        'Thunderbolt 1': 0x19,
        'Thunderbolt 2': 0x1B,
        'Thunderbolt 3': 0x1C,
      },
      endpointName: 'ddc',
      cluster: 'manuSpecificMoniBeeDDC',
      attribute: 'ddc_input',
      access: 'ALL',
      reporting: {min: 1, max: 60, change: 1},
    }),
    m.numeric({
      name: 'ddc_brightness',
      label: 'Display Brightness',
      description: 'Current display brightness',
      unit: '%',
      valueMin: 0,
      valueMax: 100,
      valueStep: 1,
      endpointNames: ['ddc'],
      cluster: 'manuSpecificMoniBeeDDC',
      attribute: 'ddc_brightness',
      access: 'ALL',
      precision: 0,
      reporting: {min: 1, max: 60, change: 1},
    }),
    m.enumLookup({
      name: 'ddc_power_mode',
      label: 'Display Power Mode',
      description: 'Currently active power mode',
      lookup: {
        'Unknown': 0x00,
        'On': 0x01,
        'Standby': 0x02,
        'Suspend': 0x03,
        'Off': 0x04,
      },
      endpointName: 'ddc',
      cluster: 'manuSpecificMoniBeeDDC',
      attribute: 'ddc_power_mode',
      access: 'ALL',
      reporting: {min: 1, max: 60, change: 1},
    }),
  ],
  exposes: [
    e.composite('send_ir_command', 'send_ir_command')
      .withDescription('Send IR Command')
      .withFeature(exposes.numeric('address', ea.SET).withValueMin(0).withValueMax(0xFFFF).withDescription('IR NEC Address'))
      .withFeature(exposes.numeric('command', ea.SET).withValueMin(0).withValueMax(0xFFFF).withDescription('IR NEC Command'))
  ],
  toZigbee: [{
    key: ['send_ir_command'],
    convertSet: async (entity, key, value, meta) => {
      const entityToUse = meta.device.getEndpoint(13);
      const payload = {
        address: value.address,
        command: value.command,
      };

      await entityToUse.command(
        'manuSpecificMoniBeeIR',
        'sendIRCommand',
        payload,
        {}
      );

      return {state: {send_ir_command: payload}};
    },
    convertGet: null,
  }],
  meta: {
    multiEndpoint: true
  },
};
module.exports = definition;