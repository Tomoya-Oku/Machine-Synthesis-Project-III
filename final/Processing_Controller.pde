import processing.serial.*;
Serial myPort;

//矢印キーの入力値
int mode = 0;

void setup()
{
    size(1200, 600); //Windowサイズ指定
    
    print(Serial.list());
    myPort = new Serial(this, "/dev/tty.usbmodem14101", 115200);
}

void draw()
{
    background(#364c97);
  
    if (keyPressed)
    {
        if (key == 'O' || key == 'o')
        {
            mode = 1;
            
            fill(#FFFFFF);
            textSize(50);
            noStroke();
            textAlign(CENTER, CENTER);
            text("ARM OPEN", 900, 300);
        }
        else if (key == 'P' || key == 'p')
        {
            mode = 2;
            
            fill(#FFFFFF);
            textSize(50);  //文字の大きさ
            noStroke();
            textAlign(CENTER, CENTER);
            text("ARM CLOSE", 900, 300);
        }
        else if (key == 'U' || key == 'u')
        {
            mode = 3;
            
            fill(#FFFFFF);
            textSize(50);  //文字の大きさ
            noStroke();
            textAlign(CENTER, CENTER);
            text("RACK UP", 900, 300);
        }
        else if (key == 'I' || key == 'i')
        {
            mode = 4;
            
            fill(#FFFFFF);
            textSize(50);  //文字の大きさ
            noStroke();
            textAlign(CENTER, CENTER);
            text("RACK DOWN", 900, 300);
        }
        
        else if (key == 'W' || key == 'w')
        {
            mode = 5;
          
            fill(#FFFFFF);   //四角形の色
            noStroke();      //枠線なし
            rectMode(CENTER);  //基準を中心にする
            rect(300, 300, 50, 200);  //四角形の座標，大きさ
            
            fill(#FFFFFF);  //三角形の色
            noStroke();  //枠線なし
            triangle(300, 150, 200, 300, 400, 300);  //三角形の頂点の座標
        }
        else if (key == 'A' || key == 'a')
        {
            mode = 6;
            
            fill(#FFFFFF);   //四角形の色
            noStroke();      //枠線なし
            rectMode(CENTER);  //基準を中心にする
            rect(300, 300, 200, 50);  //四角形の座標，大きさ
            
            fill(#FFFFFF);  //三角形の色
            noStroke();    //枠線なし
            triangle(300, 200, 300, 400, 150, 300);  //三角形の頂点の座標
        }
        else if (key == 'S' || key == 's')
        {
            mode = 7;
            
            fill(#FFFFFF);   //四角形の色
            noStroke();      //枠線なし
            rectMode(CENTER);  //基準を中心にする
            rect(300, 300, 50, 200);  //四角形の座標，大きさ
            
            fill(#FFFFFF);  //三角形の色
            noStroke();  //枠線なし
            triangle(300, 450, 200, 300, 400, 300);  //三角形の頂点の座標
        }
        else if (key == 'D' || key == 'd')
        {
            mode = 8;
            
            fill(#FFFFFF);   //四角形の色
            noStroke();      //枠線なし
            rectMode(CENTER);  //基準を中心にする
            rect(300, 300, 200, 50);  //四角形の座標，大きさ
            
            fill(#FFFFFF);  //三角形の色
            noStroke();  //枠線なし
            triangle(300, 200, 300, 400, 450, 300);  //三角形の頂点の座標
        }
        else
        {
            mode = 0;
        }
    }
      else
      {
          mode = 0;   //キーが押されていない時，0を返す
      }
    
    fill(#FFFFFF);
    textSize(20);  //文字の大きさ
    noStroke();
    textAlign(CENTER, CENTER);
    text(mode, 600, 100);
    textAlign(LEFT, CENTER);
    text("O: Open arms", 565, 180);
    text("P: Close arms", 565, 200);
    text("U: Up rack", 565, 220);
    text("I: Down rack", 565, 240);
    text("W: Forward", 565, 260);
    text("A: Left", 565, 280);
    text("S: Back", 565, 300);
    text("D: Right", 565, 320);
    
    myPort.write((char)mode); //Portに信号を伝える
}
