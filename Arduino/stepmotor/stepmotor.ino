#include <Stepper.h>
#include <MsTimer2.h>

const int stepPerRevolution = 2048;
bool timerIsrFlag = false;
unsigned int secCount;
int angle = 0;
int m_angle;

bool motorFlag = false;

Stepper Motor(stepPerRevolution, 5, 3, 4, 2);

void timerIsr();

void setup()
{
  Motor.setSpeed(15);
  Serial.begin(9600);

  MsTimer2::set(1000, timerIsr);
  MsTimer2::start();
}

void loop()
{
  // step.step(stepPerRevolution);
  // delay(500);

  // step.step(-stepPerRevolution);
  // delay(500);

  if(timerIsrFlag)
  {
    timerIsrFlag = false;

    if(!(secCount % 10))
    {
      angle = 120;
      motorFlag = true;

      // if(angle >= 360)
      // {
      //   angle = 0;
      // }
    }

    Serial.print(motorFlag);
    Serial.println(angle);
  }

  if(motorFlag)
  {
    motorFlag = false;

    m_angle = map(angle > 0? angle : 360 + angle, 0, 360, 0, 2048);

    Motor.step(m_angle); 
  }

  

  // if(Serial.available())
  // {
  //   int val = Serial.parseInt();

  //   val = map(val, 0, 360, 0, 2048);

  //   Motor.step(val);
  //   Serial.println(val);
  // }
}

void timerIsr()
{
  timerIsrFlag = true;
  secCount++;
}
