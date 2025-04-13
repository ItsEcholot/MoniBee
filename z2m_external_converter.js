const m = require('zigbee-herdsman-converters/lib/modernExtend');
const Zcl = require('zigbee-herdsman').Zcl;

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
        'ddc': 12
      }
    }),
    m.temperature({
      endpointNames: ['internal_temperature'],
      label: 'Internal Temperature',
    }),
    m.illuminance({
      endpointNames: ['veml_7700'],
      label: 'Ambient Illuminance',
      name: 'ambient_illuminance',
      precision: 0,
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
    }),
    m.enumLookup({
      name: 'ddc_power_mode',
      label: 'Display Power Mode',
      description: 'Currently active power mode',
      lookup: {
        'On': 0x01,
        'Standby': 0x02,
        'Suspend': 0x03,
        'Off': 0x04,
      },
      endpointName: 'ddc',
      cluster: 'manuSpecificMoniBeeDDC',
      attribute: 'ddc_power_mode',
      access: 'ALL',
    }),
  ],
  configure: async (device) => {
    await device.getEndpoint(10).configureReporting('msTemperatureMeasurement', [
      {attribute: 'measuredValue', minimumReportInterval: 5, maximumReportInterval: 30, reportableChange: 100},
    ]);

    await device.getEndpoint(11).configureReporting('msIlluminanceMeasurement', [
      {attribute: 'measuredValue', minimumReportInterval: 1, maximumReportInterval: 5, reportableChange: 5},
    ]);

    await device.getEndpoint(12).configureReporting('manuSpecificMoniBeeDDC', [
      {attribute: 'ddc_input', minimumReportInterval: 1, maximumReportInterval: 60, reportableChange: 1},
      {attribute: 'ddc_brightness', minimumReportInterval: 1, maximumReportInterval: 60, reportableChange: 1},
      {attribute: 'ddc_power_mode', minimumReportInterval: 1, maximumReportInterval: 60, reportableChange: 1},
    ]);
  },
  meta: {
    multiEndpoint: true
  },
};
module.exports = definition;