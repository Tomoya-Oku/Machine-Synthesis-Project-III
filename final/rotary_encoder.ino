// 2 Rotary Encoders for direction detect
int valA, valB;
int stateA, stateB;
int edgeA, edgeB;
int count;

const int threshold_ON = 600;
const int threshold_OFF = 450;
const int PHASE_A = A0; // Encoder phase A
const int PHASE_B = A1; // Encoder phase B

void setup()
{
  stateA = 0;
  stateB = 0;
  count = 0;
  Serial.begin(115200);
  Serial.println(count);
}

void loop()
{
  valA = analogRead(PHASE_A);
  valB = analogRead(PHASE_B);

  /*
  Serial.println(valA);
  Serial.println(valB);
  */

  /* edge detection */
  edgeA = 0;
  
  if ((valA > threshold_ON) && (stateA == 0))
  {
    stateA = 1; edgeA = 1; // rising edge
  }
  if ((valA < threshold_OFF) && (stateA == 1))
  {
    stateA = 0; edgeA = -1; // falling edge
  }
  edgeB = 0;
  if ((valB > threshold_ON) && (stateB == 0))
  {
    stateB = 1; edgeB = 1; // rising edge
  }
  if ((valB < threshold_OFF) && (stateB == 1))
  {
    stateB = 0; edgeB = -1; // falling edge

    /* pulse & direction count */
    if (edgeA == 1)
    { // A rising
      if (stateB) { count --; } else { count ++; }
      Serial.println(count);
    }
    if (edgeA == -1)
    { // A falling
      if (stateB) { count ++; } else { count --; }
      Serial.println(count);
    }
    if (edgeB == 1)
    { // B rising
      if (stateA) { count ++; } else { count --; }
      Serial.println(count);
    }
    if (edgeB == -1)
    { // B falling
      if (stateA) { count --; } else { count ++; }
      Serial.println(count);
    }
  }
}