//ADCDMM

#define ADC_VALUE          1     // index in the 2 dimensional LUT
#define ACTUAL_VALUE       0     // index in the 2 dimensional LUT

#define VOLT_NUM_ELEMENTS 39
#define AMP_NUM_ELEMENTS 58



// Format is [ACTUAL VALUE][ADC VALUE]
float VOLT_CAL_LUT[VOLT_NUM_ELEMENTS][2] = {
  {11.842, 11.852},
  {11.866,11.875},
  {11.871,11.879},
  {11.8843,11.891},
  {11.9025, 11.90873},
  {11.9085,11.91415},
  {11.92049,11.9237},
  {11.91842,11.9217},
  {11.924,11.929},
  {11.9361, 11.94044},
  {11.9388, 11.9431},
  {11.9468, 11.94834},
  {11.9481,11.94864},
  {11.9485,11.9517},
  {11.9486, 11.9523},
  {11.9566, 11.9599},
  {11.9597, 11.9628},
  {11.963, 11.96247},
  {11.9625, 11.9655},
  {11.9648, 11.9675},
  {11.9657,11.96854},
  {11.9663,11.9688},
  {11.969,11.9715},
  {11.9714,11.9736},
  {11.9722,11.97445},
  {11.9728,11.97514},
  {11.97442,11.9767},
  {11.97507,11.9771},
  {11.9759,11.9773},
  {11.97775,11.9799},
  {11.9785, 11.9878},
  {11.988,11.988},
  {11.992,11.992},
  {11.9941,11.9949},
  {12.015,12.0144},
  {12.017,12.0164},
  {12.02,12.019},
  {12.021,12.01996},
  {12.022,12.021},
};

// Format is [Actual Amps][Measured Amps]
float AMP_CAL_LUT[AMP_NUM_ELEMENTS][2] = {
{ 0.0253,0.0185},
{0.0732,0.0635},
{0.0442,0.0397},
{0.1230,0.1189},
{0.1463,0.1262},
{0.1663,0.1585},
{0.1708,0.1657},
{0.1758,0.1689},
{0.1796,0.1711},
{0.1809,0.1731},
{0.1826,0.1752},
{0.1835,0.1771},
{0.185,0.1872},
{0.1879,0.1826},
{0.187,0.1888},
{0.1892,0.1929},
{0.195,0.1936},
{0.211,0.2088},
{0.212,0.2112},
{0.215,0.2128},
{0.271,0.2696},
{0.2883,0.2932},
{0.2670,0.2712},
{0.2940,0.2969},
{0.3240,0.3264},
{0.355,0.3575},
{0.360,0.3623},
{.409,.410},
{.411,.412},
{.414,.4149},
{.416,.4168},
{.417,.4174},
{.45622,.44366},
{.45622,.44366},
{.45622,.44366},
{.45622,.44366},
{.475,.474},
{.480,.4769},
{.483,.480},
{.4874,.4824},
{.50536,.50195},
{.51025,.50513},
{.51082,.5067},
{.51343,.50733},
{.51542,.50867},
{.51676,.51049},
{.51898,.51153},
{.51991,.51256},
{.52325,.51622},
{.52144,.51404},
{.52664,.519},
{.53398,.5249},
{.53783,.52892},
{.54844,.53737},
{.54286,.53285},
{.55177,.54078},
{0.5518,0.5408},
};
/*
float AMP_CAL_LUT[AMP_NUM_ELEMENTS][2] = {
  {.14848, .1457},
  {.17062, .168},
  {.19927, .1968},
  {.22686, .2245},
  {.26742, .2654},
  {.34402, .3426},
  {.45742, .4568},
  {.59285, .5931},
  {.6828, .682},
  {.802, .804},
  {.8104, .8087},
  {.81176, .8127},
  {.8129, .816},
  {.8129, .816},
  {.930, .9307},
};
*/

float get_Corrected_Volt(float adc_volts);
int Find_V_Upper_Bound(float adc_volts);
float get_Corrected_Amp(float shunt_voltage);
int Find_Amp_Upper_Bound(float shunt_voltage);


float get_Corrected_Volt(float adc_volts)
{
    int index_lower_bound=0, index_upper_bound=1;
    float correct_val_lower_bound, correct_val_upper_bound, adc_val_lower_bound, adc_val_upper_bound;
    float slope, V_offset, delta_Correct_Val, delta_ADC_Val;
    float interpolated_cfactor_val, interpolated_adc_voltage;
    float distance_to_interpolate = 0;
    float corrected_voltage = 0;

    index_upper_bound = Find_V_Upper_Bound(adc_volts);
    if(index_upper_bound == 0) // voltage is greater than max value in the LUT (colder than min temp)
    {
      return adc_volts;
        // handle case of minimum temperature
        // use minimum val in LUT and calculate slope relative to 0

    }
    else index_lower_bound = index_upper_bound-1;

    adc_val_lower_bound = VOLT_CAL_LUT[index_lower_bound][ADC_VALUE]; // ADC returned value
    correct_val_lower_bound = VOLT_CAL_LUT[index_lower_bound][ACTUAL_VALUE];  // actual measured value

    adc_val_upper_bound = VOLT_CAL_LUT[index_upper_bound][ADC_VALUE];  // higher temp
    correct_val_upper_bound= VOLT_CAL_LUT[index_upper_bound][ACTUAL_VALUE]; // lower voltage

        // find closest data point, this is the point to interpolate from
    float distance_to_upper_bound = adc_val_upper_bound - adc_volts;
    float distance_to_lower_bound = adc_volts - adc_val_lower_bound;

    if(distance_to_upper_bound <= distance_to_lower_bound)
    {
        //Upper data point is closer
        distance_to_interpolate = distance_to_upper_bound;

        V_offset = VOLT_CAL_LUT[index_upper_bound][ACTUAL_VALUE];

        delta_Correct_Val = VOLT_CAL_LUT[index_upper_bound][ACTUAL_VALUE] - VOLT_CAL_LUT[index_lower_bound][ACTUAL_VALUE];
        delta_ADC_Val =  VOLT_CAL_LUT[index_upper_bound][ADC_VALUE] - VOLT_CAL_LUT[index_lower_bound][ADC_VALUE];

        slope = delta_Correct_Val / delta_ADC_Val;

        corrected_voltage = V_offset - (slope*distance_to_interpolate); // subtracting because offset is at upper bound
    }

    else
    {
        //Lower data point is closer
        distance_to_interpolate = distance_to_lower_bound;
        V_offset = VOLT_CAL_LUT[index_lower_bound][ACTUAL_VALUE];

        delta_Correct_Val = VOLT_CAL_LUT[index_upper_bound][ACTUAL_VALUE] - VOLT_CAL_LUT[index_lower_bound][ACTUAL_VALUE];
        delta_ADC_Val =  VOLT_CAL_LUT[index_upper_bound][ADC_VALUE] - VOLT_CAL_LUT[index_lower_bound][ADC_VALUE];

        slope = delta_Correct_Val / delta_ADC_Val;
        corrected_voltage = V_offset + (slope*distance_to_interpolate); // adding because offset is at lower bound
    }

    return corrected_voltage;
}

// Returns index of bounding value in LUT for a given voltage
int Find_V_Upper_Bound(float adc_volts)
{
  int index = 0;
  bool found = false;
  float voltage_at_index;

  while(!found)
  {
      voltage_at_index = VOLT_CAL_LUT[index][ADC_VALUE];
      if(voltage_at_index >= adc_volts)
      { return index; }
      else
      {
        index += 1;
        if( index >= VOLT_NUM_ELEMENTS){ return (VOLT_NUM_ELEMENTS -1); }
        continue;
      }
  }
}

float get_Corrected_Amp(float raw_amps)
{

    //printf("raw amp: %.4f\t\t", raw_amps);
    int index_lower_bound=0, index_upper_bound=1;
    float actual_val_lower_bound, actual_val_upper_bound, meas_val_lower_bound, meas_val_upper_bound;
    float slope, Amp_offset, delta_Amp_Val, delta_Meas_Val;
    float interpolated_correct_val, interpolated_adc_voltage;
    float distance_to_interpolate = 0;
    float corrected_amp = 0;
    float correction_factor=0;

    index_upper_bound = Find_Amp_Upper_Bound(raw_amps);
    if(index_upper_bound == 0) // voltage is greater than max value in the LUT (colder than min temp)
    {
      return raw_amps;
    }
    else index_lower_bound = index_upper_bound-1;

    meas_val_lower_bound = AMP_CAL_LUT[index_lower_bound][ADC_VALUE]; // ADC returned value
    actual_val_lower_bound = AMP_CAL_LUT[index_lower_bound][ACTUAL_VALUE];  // actual measured value

    meas_val_upper_bound = AMP_CAL_LUT[index_upper_bound][ADC_VALUE];  // higher temp
    actual_val_upper_bound= AMP_CAL_LUT[index_upper_bound][ACTUAL_VALUE]; // lower voltage

        // find closest data point
    float distance_to_upper_bound = meas_val_upper_bound - raw_amps;
    float distance_to_lower_bound = raw_amps - meas_val_lower_bound;

    if(distance_to_upper_bound <= distance_to_lower_bound)
    {
        //Upper data point is closer
        distance_to_interpolate = distance_to_upper_bound;

        Amp_offset = AMP_CAL_LUT[index_upper_bound][ACTUAL_VALUE];

        delta_Amp_Val = AMP_CAL_LUT[index_upper_bound][ACTUAL_VALUE] -AMP_CAL_LUT[index_lower_bound][ACTUAL_VALUE];
        delta_Meas_Val =  AMP_CAL_LUT[index_upper_bound][ADC_VALUE] - AMP_CAL_LUT[index_lower_bound][ADC_VALUE];

        slope = delta_Amp_Val / delta_Meas_Val;

        corrected_amp = Amp_offset - (slope*distance_to_interpolate); // subtracting because offset is at upper bound
    }
    else
    {
              //printf("\t\tLOWER IS CLOSER\n");
        //Lower data point is closer
        distance_to_interpolate = distance_to_lower_bound;
        Amp_offset = AMP_CAL_LUT[index_lower_bound][ACTUAL_VALUE];

        delta_Amp_Val = AMP_CAL_LUT[index_upper_bound][ACTUAL_VALUE] - AMP_CAL_LUT[index_lower_bound][ACTUAL_VALUE];
        delta_Meas_Val =  AMP_CAL_LUT[index_upper_bound][ADC_VALUE] - AMP_CAL_LUT[index_lower_bound][ADC_VALUE];

        slope = delta_Amp_Val / delta_Meas_Val;
        corrected_amp = Amp_offset + (slope*distance_to_interpolate); // adding because offset is at lower bound
    }
    //printf("corrected amp: %.4f\n", corrected_amp);
    return corrected_amp;
}

// Returns index of bounding value in LUT for a given voltage
int Find_Amp_Upper_Bound(float shunt_voltage)
{
  int index = 0;
  bool found = false;
  float voltage_at_index;

  while(!found)
  {
      voltage_at_index = AMP_CAL_LUT[index][ADC_VALUE];
      if(voltage_at_index >= shunt_voltage)
      { return index; }
      else
      {
        index += 1;
        if( index >= AMP_NUM_ELEMENTS){ return (AMP_NUM_ELEMENTS -1); }
        continue;
      }
  }
}
