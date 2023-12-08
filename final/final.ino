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
}

/*左右タイヤ[0: LEFT, 1: RIGHT]*/
enum LR
{
	LEFT,
	RIGHT
}

/*モード・フェーズ初期値*/
Mode mode = TEST;
Phase phase = PUSHING;

/*超音波センサ関連パラメータ*/
const int DISTANCE_COUNT = 1;  // 距離検知の基準回数（この回数だけ基準値を下回ったら停止）
int AIR_TEMPERATURE = 20;	   // 初期化により変更される
double SPEED_OF_SOUND = 343.5; // 気温20℃での値
int distance_counter = 0;	   // 距離検知回数

/*フォトリフレクタ関連パラメータ*/
const int BLACK_LINE = 500; // 黒線検知の輝度基準値
const int BLACK_COUNT = 1;	// 黒線検知の基準回数（この回数だけ基準値を下回ったら停止）
const int BLACK_CUP = 500;	// 黒いドリンクの輝度基準値
int black_counter = 0;		// 黒線検知回数

/*温度関連パラメータ*/
const int HOT_VALUE = 40;						 // テーブルがHOTと判断する基準値[℃]
const float B = 3950.0;							 // サーミスタのB定数
const float R0 = 10000.0;						 // サーミスタの25度での抵抗値（カタログ値）
const float RD = 10000.0;						 // 検知抵抗の抵抗値
const float TK = 273.15;						 // 0度=273.15ケルビン
const int CHECKING_TEMPERATURE_TIME = 10 * 1000; // 温度を調べる時間[ms]

/*アーム関連パラメータ*/
const int CARRYING_DISTANCE = 50; // ドリンクを運ぶ際に近づく距離[mm]
bool arm_is_open = true;		  // アームが開いているかどうか
bool arm_is_down = true;		  // アームが下がっているかどうか

/*タイヤ関連パラメータ*/
const int SHAFT_LENGTH = 10;	// シャフト長[cm]
const int TIRE_DIAMETER = 3;	// タイヤ直径[cm]
const int CORRECTION_SPEED = 1; // 直進補正をする際の重み
int speed[2] = {255, 255}

/*ロータリーエンコーダ関連*/
const int threshold_ON = 600;
const int threshold_OFF = 450;
/*回転量*/
double rotation_amount[2] = {0, 0};
/*現在の座標[x, y, angle]*/
double position[3] = {0, 0, M_PI / 2}; // 角度は左曲がりで+
/*1ループ前の座標*/
double before_position[3] = {0, 0, M_PI / 2}

/**********ピン番号***********/
/*LED*/
const int LED = 4;
/*超音波センサ*/
const int ECHO = 2;
const int TRIG = 3;
/*モータドライバ左*/
const int AIN1 = 7;
const int AIN2 = 8;
const int PWMA = 9;
/*モータドライバ左*/
const int BIN1 = 11;
const int BIN2 = 13;
const int PWMB = 10;
/*フォトリフレクタ*/
const int PHOTO_REF = A0; // 黒線検知用
/*サーミスタ*/
const int THERMISTOR = A1;
/*ロータリーエンコーダ左タイヤ*/
const int L_PHASE_A = A2;
const int L_PHASE_B = A3;
/*ロータリーエンコーダ右タイヤ*/
const int R_PHASE_A = A4;
const int R_PHASE_B = A5;

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

	/*気温を取得し，音速を計算*/
	AIR_TEMPERATURE = getTemp();
	SPEED_OF_SOUND = 331.5 + 0.6 * AIR_TEMPERATURE;

	/*アームを開く*/
	armOpen();

	Report("Setup has finished.");
}

void loop()
{
	/*シリアルモニタでコマンド受付*/
	if (Serial.available())
	{
		String command = Serial.readStringUntil('\n'); // シリアルから文字列を受信
		command.toLowerCase();						   // 入力された文字列を小文字に
		Response(command);
	}

	/*自律制御*/
	if (mode == AUTO)
	{
		/*現在の座標を記録*/
		before_position = position;

		calculatePosition(); // 位置情報を更新

		switch (phase)
		{
		/*最初に2個のドリンクを押すフェーズ*/
		case PUSHING:
		{
			/*カウンターまでドリンクを入れたとき*/
			if (isInCounter())
			{
				Report("COMPLETE PUSHING -> FINDING");
				phase = FINDING;
			}
			else
			{
				// armOpen();
				goStraight(); // 直進
			}
			break;
		}

		/*他のドリンクを探すフェーズ*/
		case FINDING:
		{
			findDrink(); // ドリンクを探す

			Report("COMPLETE FINDING -> APPROACHING");
			phase = APPROACHING;
			break;
		}

		/*検出したドリンクまで向かうフェーズ*/
		case APPROACHING:
		{
			/*適当な距離までドリンクに近づく*/
			if (getDistance(TRIG, ECHO) <= CARRYING_DISTANCE)
			{
				distance_counter++; // ノイズ対策
				if (distance_counter >= DISTANCE_COUNT)
				{
					Report("COMPLETE APPROACHING -> LIFTING");
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

		/*ドリンクを持ち上げるフェーズ*/
		case LIFTING:
		{
			armClose(); // アームを閉じる
			delay(1000);
			armUp(); // アームを上げる
			phase = SEARCHING;
			break;
		}

		/*ドリンクを置くテーブルを探すフェーズ*/
		case SEARCHING:
		{
			findTable(); // テーブルを探す
			Report("COMPLETE SEARCING -> CARRYING");
			phase = CARRYING;
			break;
		}

		/*テーブルまでドリンクを運ぶフェーズ*/
		case CARRYING:
		{
			/*適当な距離までテーブルに近づく*/
			if (getDistance(TRIG, ECHO) <= CARRYING_DISTANCE)
			{
				distance_counter++; // ノイズ対策
				if (distance_counter >= DISTANCE_COUNT)
				{
					Report("COMPLETE CARRYING -> CHECKING");
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

		/*テーブルの温度を調べるフェーズ*/
		case CHECKING:
		{
			delay(CHECKING_TEMPERATURE_TIME);
			int temp = getTemp();

			/*テーブルの温度がHOTであればLEDをオンに*/
			if (temp >= HOT_VALUE)
			{
				LED_ON();
			}
			else
			{
				LED_OFF();
			}

			/*テーブルの温度とドリンクの色が一致*/
			if ((temp >= HOT_VALUE && isBlack()) || (temp < HOT_VALUE && !isBlack()))
			{
				Report("TEMPERATURE IS CORRECT");
				phase = PUTTING;
			}
			/*テーブルの温度とドリンクの色が一致しない*/
			else
			{
				Report("TEMPERATURE IS WRONG");
				phase = SEARCHING;
			}
			break;
		}

		/*テーブルにドリンクを置くフェーズ*/
		case PUTTING:
		{
			armDown(); // アームを下ろす
			armOpen(); // アームを開く
			goBack();  // 後退する

			Report("COMPLETE PUTTING");

			achievement_flag++; // 達成フラグを1増やす
			/*2つのドリンクを正しくテーブルに乗せた*/
			if (achievement_flag >= 2)
			{
				Report("COMPLETE MISSION");
				phase = SUCCESS;
			}
			else
			{
				Report("-> FINDING");
				phase = FINDING;
			}
			break;
		}

		/*成功後のフェーズ*/
		case SUCCESS:
		{
			Success();
			break;
		}
		}
	}
	/*遠隔操縦*/
	else if (mode == MANUAL)
	{
		// TODO: 要編集
	}
	/*テスト*/
	else if (mode == TEST)
	{
	}
}

/*アームを開く*/
void armOpen()
{
	/*アームが開いていないとき*/
	if (!arm_is_open)
	{
		// TODO: 要編集
	}
}

/*アームを閉じる*/
void armClose()
{
	/*アームが開いているとき*/
	if (arm_is_open)
	{
		// TODO: 要編集
	}
}

/*アームを上げる*/
void armUp()
{
	/*アームが下がっているとき*/
	if (arm_is_down)
	{
		// TODO: 要編集
	}
}

/*アームを下げる*/
void armDown()
{
	/*アームが上がっているとき*/
	if (!arm_is_down)
	{
		// TODO: 要編集
	}
}

/*LEDをON*/
void LED_ON()
{
	digitalWrite(LEDR, HIGH);
}

/*LEDをOFF*/
void LED_OFF()
{
	digitalWrite(LEDR, LOW);
}

/*カウンター内に入ったかどうか*/
bool isInCounter()
{
	/*黒線を検知*/
	if (getPHRBValue() <= BLACK_VALUE)
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
void goStraight(int back)
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

/*補正しつつ後退*/
void goBack()
{
	/*左に曲がっている*/
	if (positon[Angle] > before_position[Angle])
	{
		speed[RIGHT] = speed[RIGHT] + CORRECTION_SPEED;
	}
	/*右に曲がっている*/
	else if (positon[Angle] < before_position[Angle])
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

/*与えられた角度（-180[deg]~180[deg]に回転）*/
void rotate(int angle)
{
	// TODO: 要編集
}

/*ドリンクの色を判定*/
bool isBlack()
{
	/*ドリンクが黒*/
	if (getPHRFValue() <= BLACK_CUP)
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
int getPHRFValue()
{
	return analogRead(PHRF);
}

/*超音波センサで壁までの距離[mm]を計算*/
double getDistance(int trig, int echo)
{
	digitalWrite(trig, LOW);
	delayMicroseconds(2);
	digitalWrite(trig, HIGH);
	delayMicroseconds(10);
	digitalWrite(trig, LOW);
	double duration = pulseIn(echo, HIGH); // 往復にかかった時間が返却される[ms]

	if (duration > 0)
	{
		duration = duration / 2; // 往路にかかった時間[ms]
		double distance = duration * SPEED_OF_SOUND * 100 / 100000;
		return distance;
	}
}

/*温度[℃]を取得*/
int getTemp()
{
	float thrm = analogRead(THRM);
	float Rt = RD * thrm / (1023 - thrm);
	float Tbar = 1 / B * log(Rt / R0) + 1 / (TK + 25);
	float T = 1 / Tbar;
	float Tdeg = T - TK;

	return Tdeg;
}

/*タイヤの回転量を取得*/
void getRotation()
{
	int valA[2] = {0, 0};
	valA[LEFT] = analogRead(L_PHASE_A);
	valA[RIGHT] = analogRead(R_PHASE_A);

	int valB[2] = {0, 0};
	valB[LEFT] = analogRead(L_PHASE_B);
	valB[RIGHT] = analogRead(R_PHASE_B);

	int stateA[2] = {0, 0};
	int stateB[2] = {0, 0};
	int edgeA[2] = {0, 0};
	int edgeB[2] = {0, 0};

	/*左右*/
	for (int i = 0; i <= 1; i++)
	{
		/* edge detection */
		if ((valA[i] > threshold_ON) && (stateA[i] == 0))
		{
			stateA[i] = 1;
			edgeA[i] = 1; // rising edge
		}
		if ((valA[i] < threshold_OFF) && (stateA[i] == 1))
		{
			stateA[i] = 0;
			edgeA[i] = -1; // falling edge
		}
		if ((valB[i] > threshold_ON) && (stateB[i] == 0))
		{
			stateB[i] = 1;
			edgeB[i] = 1; // rising edge
		}
		if ((valB[i] < threshold_OFF) && (stateB[i] == 1))
		{
			stateB[i] = 0;
			edgeB[i] = -1; // falling edge

			/* pulse & direction count */
			if (edgeA[i] == 1)
			{ // A rising
				if (stateB[i])
				{
					rotation_amount[i]--;
				}
				else
				{
					rotation_amount[i]++;
				}
			}
			if (edgeA[i] == -1)
			{ // A falling
				if (stateB[i])
				{
					rotation_amount[i]++;
				}
				else
				{
					rotation_amount[i]--;
				}
			}
			if (edgeB[i] == 1)
			{ // B rising
				if (stateA[i])
				{
					rotation_amount[i]++;
				}
				else
				{
					rotation_amount[i]--;
				}
			}
			if (edgeB[i] == -1)
			{ // B falling
				if (stateA[i])
				{
					rotation_amount[i]--;
				}
				else
				{
					rotation_amount[i]++;
				}
			}
		}
	}
}

/*位置計算*/
void calculatePosition()
{
	if (rotation_amount[LEFT] != rotation_amount[RIGHT])
	{
		dtheta = (rotation_amount[RIGHT] - rotation_amount[LEFT]) / (2 * (SHAFT_LENGTH / 2));
		rho = ((rotation_amount[RIGHT] + rotation_amount[LEFT]) / (rotation_amount[RIGHT] - rotation_amount[LEFT])) * (SHAFT_LENGTH / 2);
		dl = 2 * rho * sin(dtheta / 2);
		dx = dl * cos(dtheta / 2);
		dy = dl * sin(dtheta / 2);

		position[X] = position[X] + dx * sin(dtheta) + dy * cos(dtheta);
		position[Y] = position[Y] + dx * cos(dtheta) - dy * sin(dtheta);
		position[Angle] = position[Angle] + dtheta;
	}
}

/*成功後の動作*/
void Success()
{
	digitalWrite(LEDR, HIGH);
	delay(200);
	digitalWrite(LEDR, LOW);
	delay(200);
}

/*シリアルモニタにて状況報告*/
void Report(String message)
{
	Serial.print("\n[Report] ");
	Serial.print(message);
}

/*シリアルモニタでコマンドに対して応答*/
void Response(String command)
{
	if (command == "phase" || command == "ph")
	{
		Serial.print("\n[Phase] ");
		Serial.print(phase);
	}
	else if (command == "mode")
	{
		Serial.print("\n[Mode] ");
		Serial.print(mode);
	}
	else if (command == "mode_change auto" || command == "mc auto")
	{
		mode = AUTO;
		phase = PUSHING;
	}
	else if (command == "mode_change manual" || command == "mc manual" || command == "mc man")
	{
		mode = MANUAL;
	}
	else if (command == "mode_change test" || command == "mc test")
	{
		mode = TEST;
	}
	else if (command == "distance" || command == "dist")
	{
		Serial.print("\n[Distance] ");
		Serial.print(getDistance(TRIG, ECHO));
		Serial.print("[mm]");
	}
	else if (command == "brightness" || command == "brt")
	{
		Serial.print("\n[Brightness] ");
		Serial.print(getBrightness());
	}
	else if (command == "temperature" || command == "temp")
	{
		Serial.print("\n[Temperature] ");
		Serial.print(getTemp());
		Serial.print("[℃]");
	}
	else if (command == "distance_counter" || command == "dc")
	{
		Serial.print("\n[Distance Counter] ");
		Serial.print(distance_counter);
	}
	else if (command == "black_line_counter" || command == "blc")
	{
		Serial.print("\n[Black Line Counter] ");
		Serial.print(black_counter);
	}
	else if (command == "air_temperature" || command == "atemp")
	{
		Serial.print("\n[Air temperature] ");
		Serial.print(AIR_TEMPERATURE);
		Serial.print("[℃]");
	}
	else if (command == "speed of sound" || command == "ss")
	{
		Serial.print("\n[Speed of sound] ");
		Serial.print(SPEED_OF_SOUND);
		Serial.print("[m/s]]");
	}
	else
	{
		Serial.print("\nI don't know the meaning of \"");
		Serial.print(command);
		Serial.print("\"");
	}
}