#include <math.h>
#include <Servo.h>

/*動作モード*/
enum Mode
{
	AUTO,	// 自動制御
	MANUAL, // 遠隔操縦
	TEST	// テスト
};

/*自動制御におけるフェーズ*/
enum Phase
{
 PREPARATION,
	PUSHING,	 // カウンターに2つのドリンクを押し込むフェーズ
	FINDING,	 // テーブルに置くドリンクを発見するフェーズ
	APPROACHING, // 発見したドリンクに近づくフェーズ
	LIFTING,	 // ドリンクを持ち上げるフェーズ
	SEARCHING,	 // テーブルを探すフェーズ
	CARRYING,	 // テーブルまでドリンクを運ぶフェーズ
	CHECKING,	 // テーブルの温度とドリンクの色が一致しているか調べるフェーズ
	PUTTING,	 // テーブルにドリンクを置くフェーズ
	SUCCESS		 // すべてのミッション成功後に繰り返すフェーズ
};

/*位置情報[x, y, angle]*/
enum Position
{
	X,
	Y,
	Angle
};

/*左右タイヤ[0: LEFT, 1: RIGHT]*/
enum LR
{
	LEFT,
	RIGHT
};

/*モード・フェーズ初期値*/
Mode mode = TEST;
Phase phase = PUSHING;

/*超音波センサ関連パラメータ*/
const int DISTANCE_COUNT = 1;  // 距離検知の基準回数（この回数だけ基準値を下回ったら停止）
int AIR_TEMPERATURE = 20;	   // 初期化により変更される
double SPEED_OF_SOUND = 343.5; // 気温20℃での値
int distance_counter = 0;	   // 距離検知回数

/*黒線検知フォトリフレクタ関連パラメータ*/
const int BLACK_LINE_VALUE = 600; // 黒線検知の輝度基準値
const int BLACK_LINE_COUNT = 1;	// 黒線検知の基準回数（この回数だけ基準値を下回ったら停止）
int black_counter = 0;		// 黒線検知回数

/*色検知フォトリフレクタ関連パラメータ*/
const int BLACK_CUP_VALUE = 500;	// 黒いドリンクの輝度基準値

/*温度関連パラメータ*/
const int HOT_TEMP = 35;						 // テーブルがHOTと判断する基準値[℃]
const int CHECKING_TEMPERATURE_TIME = 10 * 1000; // 温度を調べる時間[ms]

/*アーム関連パラメータ*/
const int DISTANCE_FOR_CARRYING = 50; // ドリンクを運ぶ際に近づく距離[mm]
bool arm_is_open = true;		  // アームが開いているかどうか
bool arm_is_down = true;		  // アームが下がっているかどうか

/*タイヤ関連パラメータ*/
const int SHAFT_LENGTH = 10;	// シャフト長[cm]
const int TIRE_DIAMETER = 3;	// タイヤ直径[cm]
const int CORRECTION_SPEED = 1; // 直進補正をする際の重み
int speed[2] = {255, 255};
const int TURN_TIME = 1500;

/*ロータリーエンコーダ関連*/
const int threshold_ON = 600;
const int threshold_OFF = 450;
/*回転量*/
double rotation_amount[2] = {0, 0};
/*現在の座標[x, y, angle]*/
double position[3] = {0, 0, M_PI / 2}; // 角度は左曲がりで+
/*1ループ前の座標*/
double before_position[3] = {0, 0, M_PI / 2};

/*サーボ関連*/
Servo servo_for_arms;
Servo servo_for_rack;
const int OPEN_ARM_ANGLE = 0;
const int CLOSE_ARM_ANGLE = 180;
const int UP_RACK_ANGLE = 180;
const int DOWN_RACK_ANGLE = 0;

/**********ピン番号***********/
/*超音波センサ*/
const int ECHO = 2;
const int TRIG = 3;
/*LED*/
const int LED = 4;
/*サーボモーター*/
const int SERVO_FOR_ARMS = 5;
const int SERVO_FOR_RACK = 6;
/*モータドライバ左*/
const int AIN1 = 7;
const int AIN2 = 8;
const int PWMA = 9;
/*モータドライバ左*/
const int BIN1 = 10;
const int BIN2 = 11;
const int PWMB = 12;
/*フォトリフレクタ*/
const int PHOTO_REFLECTOR_FOR_BLACK_LINE = A0; // 黒線検知用
const int PHOTO_REFLECTOR_FOR_CUP_COLOR = A1; // カップの色
/*サーミスタ*/
const int THERMISTOR = A2;
/*ロータリーエンコーダ左タイヤ*/
const int L_PHASE = A3;
/*ロータリーエンコーダ右タイヤ*/
const int R_PHASE = A4;

/**********グローバル変数***********/
int achievement_flag = 0; // テーブル達成状況

void setup()
{
	Serial.begin(115200);

	/*超音波センサ*/
	pinMode(ECHO, INPUT);
	pinMode(TRIG, OUTPUT);

	/*モータドライバ*/
	pinMode(AIN1, OUTPUT);
	pinMode(AIN2, OUTPUT);
	pinMode(BIN1, OUTPUT);
	pinMode(BIN2, OUTPUT);
	pinMode(PWMA, OUTPUT);
	pinMode(PWMB, OUTPUT);

	/*LED*/
	pinMode(LED, OUTPUT);

	/*サーボ*/
	servo_for_arms.attach(SERVO_FOR_ARMS);
	servo_for_rack.attach(SERVO_FOR_RACK);

	/*気温を取得し，音速を計算*/
	AIR_TEMPERATURE = getTemp();
	SPEED_OF_SOUND = 331.5 + 0.6 * AIR_TEMPERATURE;

  LED_ON();
  delay(500);
  LED_OFF();
}

void loop()
{
  int temp = getTemp();
  Serial.write(temp);

	/*自律制御*/
	if (mode == AUTO)
	{
		/*現在の座標を記録*/
    for (int i = 0; i < 3; i++)
    {
        before_position[i] = position[i];
    }

		calculatePosition(); // 位置情報を更新

  turnRight();
		// armOpen();
		goStraight(); // 直進
  delay(1000);
  turnLeft();

  
  while(isInCounter())
  {
    goStraight();
  }

  

  /*
		switch (phase)
		{
   case PREPARATION:
   {
     turnRight();
					// armOpen();
					goStraight(); // 直進
     delay(1000);
     turnLeft();

     phase = PUSHING;
     break;
   }

			case PUSHING:
			{
				if (isInCounter())
				{
					phase = FINDING;
				}
				else
				{
					goStraight(); // 直進
				}
				break;
			}

			case FINDING:
			{
				findDrink(); // ドリンクを探す
				phase = APPROACHING;
				break;
			}

			case APPROACHING:
			{
				if (getDistance(temp) <= DISTANCE_FOR_CARRYING)
				{
					distance_counter++; // ノイズ対策
					if (distance_counter >= DISTANCE_COUNT)
					{
						distance_counter = 0; // リセット
						phase = LIFTING;
					}
				}
				else
				{
					goStraight(); // 直進
				}
				break;
			}

			case LIFTING:
			{
				armClose(); // アームを閉じる
				delay(1000);
				armUp(); // アームを上げる
				phase = SEARCHING;
				break;
			}

			case SEARCHING:
			{
				findTable(); // テーブルを探す
				phase = CARRYING;
				break;
			}

			case CARRYING:
			{
				if (getDistance(temp) <= DISTANCE_FOR_CARRYING)
				{
					distance_counter++; // ノイズ対策
					if (distance_counter >= DISTANCE_COUNT)
					{
						distance_counter = 0; // リセット
						phase = CHECKING;
					}
				}
				else
				{
					goStraight(); // 直進
				}
				break;
			}

			case CHECKING:
			{
				delay(CHECKING_TEMPERATURE_TIME);
				int temp = getTemp();

				if (temp >= HOT_TEMP)
				{
					LED_ON();
				}
				else
				{
					LED_OFF();
				}

				if ((temp >= HOT_TEMP && DrinkIsBlack()) || (temp < HOT_TEMP && !DrinkIsBlack()))
				{
					phase = PUTTING;
				}
				else
				{
					phase = SEARCHING;
				}
				break;
			}

			case PUTTING:
			{
				armDown(); // アームを下ろす
				armOpen(); // アームを開く
				goBack();  // 後退する

				achievement_flag++; // 達成フラグを1増やす
				if (achievement_flag >= 2)
				{
					phase = SUCCESS;
				}
				else
				{
					phase = FINDING;
				}
				break;
			}

			case SUCCESS:
			{
				Success();
				break;
			}
		}
  */
	}
	/*遠隔操縦*/
	else if (mode == MANUAL)
	{
    int temp = getTemp();
    Serial.println(temp);

		char data = "";

		/*シリアルモニタでコマンド受付*/
		if (Serial.available() > 0)
		{
			data = Serial.read(); // シリアルから文字列を受信
		}

    int cmd = (int) data;

		switch (cmd)
		{
      case 0:
        setSpeed(0, 0);
        LED_OFF();
        break;
      case 1:
        armOpen();
        break;
      case 2:
        armClose();
        break;
      case 3:
        armUp();
        break;
      case 4:
				armDown();
				break;
			case 5:
				setSpeed(255, 255);
				break;
			case 6:
				setSpeed(-255, 255);
				break;
			case 7:
				setSpeed(-255, -255);
				break;
			case 8:
				setSpeed(255, -255);
				break;
      case 9:
        LED_ON();
        break;
      case 10:
        LED_ON();
        break;
			default:
				break;
		}
	}
  else if (mode == TEST)
  {
    Serial.println("x: ");
    Serial.print(position[X]);
    Serial.println("\ty: ");
    Serial.print(position[Y]);
    Serial.println("\tangle: : ");
    Serial.print(position[Angle]);
    
    goStraight();
  }
}

/*アームを開く*/
void armOpen()
{
	servo_for_arms.write(OPEN_ARM_ANGLE);
}

/*アームを閉じる*/
void armClose()
{
	servo_for_arms.write(CLOSE_ARM_ANGLE);
}

/*アームを上げる*/
void armUp()
{
	servo_for_rack.write(UP_RACK_ANGLE);
}

/*アームを下げる*/
void armDown()
{
	servo_for_rack.write(DOWN_RACK_ANGLE);
}

/*LEDをON*/
void LED_ON()
{
	digitalWrite(LED, HIGH);
}

/*LEDをOFF*/
void LED_OFF()
{
	digitalWrite(LED, LOW);
}

/*カウンター内に入ったかどうか*/
bool isInCounter()
{
	/*黒線を検知*/
	if (getFloorColor() > BLACK_LINE_VALUE)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*ドリンクを探す→探し終えたら終了*/
void findDrink()
{
	// TODO: 要議論（どうやって探すか，既に運び終わったドリンクをどうやって見分けるか）・要編集
 while(getDistance() < 2000)
 {
   setSpeed(255, -255);
 }
}

/*テーブルを探す→探し終えたら終了*/
void findTable()
{
	// TODO: 要議論（どうやって探すか，既にドリンクが置いてあるテーブルをどうやって見分けるか，目の前にあるテーブルともう一つのテーブルをどうやって見分けるか）・要編集
 
}

/*両タイヤを与えられたスピード（-255~255）に設定*/
void setSpeed(int L_speed, int R_speed)
{
	speed[LEFT] = L_speed;
	speed[RIGHT] = R_speed;

	if (L_speed >= 0)
	{
		digitalWrite(AIN1, HIGH);
		digitalWrite(AIN2, LOW);
		analogWrite(PWMA, L_speed);
	}
	else
	{
		digitalWrite(AIN1, LOW);
		digitalWrite(AIN2, HIGH);
		analogWrite(PWMA, -L_speed);
	}

	if (R_speed >= 0)
	{
		digitalWrite(BIN1, HIGH);
		digitalWrite(BIN2, LOW);
		analogWrite(PWMB, R_speed);
	}
	else
	{
		digitalWrite(BIN1, LOW);
		digitalWrite(BIN2, HIGH);
		analogWrite(PWMB, -R_speed);
	}
}

/*補正しつつ直進*/
void goStraight()
{
	/*左に曲がっている*/
	if (position[Angle] > before_position[Angle])
	{
		speed[RIGHT] = speed[RIGHT] - CORRECTION_SPEED;
	}
	/*右に曲がっている*/
	else if (position[Angle] < before_position[Angle])
	{
		speed[LEFT] = speed[LEFT] - CORRECTION_SPEED;
	}
	/*曲がっていない*/
	else
	{
		speed[LEFT] = 255;
		speed[RIGHT] = 255;
	}

	setSpeed(speed[LEFT], speed[RIGHT]);
}

void turnLeft()
{
  setSpeed(-255, 255);
  delay(TURN_TIME);
}

void turnRight()
{
 setSpeed(255, -255);
 delay(TURN_TIME);
}

/*補正しつつ後退*/
void goBack()
{
	/*左に曲がっている*/
	if (position[Angle] > before_position[Angle])
	{
		speed[RIGHT] = speed[RIGHT] + CORRECTION_SPEED;
	}
	/*右に曲がっている*/
	else if (position[Angle] < before_position[Angle])
	{
		speed[LEFT] = speed[LEFT] + CORRECTION_SPEED;
	}
	/*曲がっていない*/
	else
	{
		speed[LEFT] = -255;
		speed[RIGHT] = -255;
	}

	setSpeed(speed[LEFT], speed[RIGHT]);
}

/*ドリンクの色を判定*/
bool DrinkIsBlack()
{
	/*ドリンクが黒*/
	if (getDrinkColor() > BLACK_CUP_VALUE)
	{
		return true;
	}
	/*ドリンクが白*/
	else
	{
		return false;
	}
}

/*フォトリフレクタで輝度を取得*/
int getDrinkColor()
{
	return analogRead(PHOTO_REFLECTOR_FOR_CUP_COLOR);
}

int getFloorColor()
{
	return analogRead(PHOTO_REFLECTOR_FOR_BLACK_LINE);
}

/*超音波センサで壁までの距離[mm]を計算*/
double getDistance(int temp)
{
	digitalWrite(TRIG, LOW);
	delayMicroseconds(2);
	digitalWrite(TRIG, HIGH);
	delayMicroseconds(10);
	digitalWrite(TRIG, LOW);
	double duration = pulseIn(ECHO, HIGH); // 往復にかかった時間が返却される[ms]

	if (duration > 0)
	{
		duration = duration / 2; // 往路にかかった時間[ms]
		double distance = duration * getSpeedOfSound(temp) * 100 / 100000;
		return distance;
	}
}

/*温度[℃]を取得*/
int getTemp()
{
	float B = 3950.0;							 // サーミスタのB定数
	float R0 = 10000.0;						 // サーミスタの25度での抵抗値（カタログ値）
	float RD = 2000.0;						 // 検知抵抗の抵抗値
	float TK = 273.15;						 // 0度=273.15ケルビン

	float thrm = analogRead(THERMISTOR);
	float Rt = RD * thrm / (1023 - thrm);
	float Tbar = 1 / B * log(Rt / R0) + 1 / (TK + 25);
	float T = 1 / Tbar;
	float Tdeg = T - TK;

	return Tdeg;
}

double getSpeedOfSound(int temp)
{
  return 331.5 + 0.6 * temp;  
}

/*タイヤの回転量を取得*/
//TODO: 修正
void getRotation()
{
	int val[2] = {0, 0};
	val[LEFT] = analogRead(L_PHASE);
	val[RIGHT] = analogRead(R_PHASE);

	int state[2] = {0, 0};
	int edge[2] = {0, 0};

	/*左右*/
	for (int i = 0; i <= 1; i++)
	{
		/* edge detection */
		if ((val[i] > threshold_ON) && (state[i] == 0))
		{
			state[i] = 1;
			edge[i] = 1; // rising edge
		}
		if ((val[i] < threshold_OFF) && (state[i] == 1))
		{
			state[i] = 0;
			edge[i] = -1; // falling edge
		}

		/* pulse & direction count */
		if (edge[i] == 1)
		{ // A rising
			if (state[i])
			{
				rotation_amount[i]--;
			}
			else
			{
				rotation_amount[i]++;
			}
		}
		if (edge[i] == -1)
		{ // A falling
			if (state[i])
			{
				rotation_amount[i]++;
			}
			else
			{
				rotation_amount[i]--;
			}
		}
	}
}

/*位置計算*/
void calculatePosition()
{
	if (rotation_amount[LEFT] != rotation_amount[RIGHT])
	{
		double dtheta = (rotation_amount[RIGHT] - rotation_amount[LEFT]) / (2 * (SHAFT_LENGTH / 2));
		double rho = ((rotation_amount[RIGHT] + rotation_amount[LEFT]) / (rotation_amount[RIGHT] - rotation_amount[LEFT])) * (SHAFT_LENGTH / 2);
		double dl = 2 * rho * sin(dtheta / 2);
		double dx = dl * cos(dtheta / 2);
		double dy = dl * sin(dtheta / 2);

		position[X] = position[X] + dx * sin(dtheta) + dy * cos(dtheta);
		position[Y] = position[Y] + dx * cos(dtheta) - dy * sin(dtheta);
		position[Angle] = position[Angle] + dtheta;
	}
}

/*成功後の動作*/
void Success()
{
	digitalWrite(LED, HIGH);
	delay(200);
	digitalWrite(LED, LOW);
	delay(200);
}