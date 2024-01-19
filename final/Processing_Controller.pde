import processing.serial.*;
Serial myPort;

int mode = 0;
int temp = 0;

PImage bulb;

void setup()
{
    size(1200, 600); //Windowサイズ指定
    bulb = loadImage("bulb.png");
    
    print(Serial.list());
    myPort = new Serial(this, "/dev/tty.usbmodem14201", 115200);
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
            textSize(200);
            noStroke();
            textAlign(CENTER, CENTER);
            text("◀▶", 900, 300);
        }
        else if (key == 'P' || key == 'p')
        {
            mode = 2;
            
            fill(#FFFFFF);
            textSize(200);  //文字の大きさ
            noStroke();
            textAlign(CENTER, CENTER);
            text("▶◀", 900, 300);
        }
        else if (key == 'U' || key == 'u')
        {
            mode = 3;
            
            fill(#FFFFFF);
            textSize(200);  //文字の大きさ
            noStroke();
            textAlign(CENTER, CENTER);
            text("▲", 900, 300);
        }
        else if (key == 'I' || key == 'i')
        {
            mode = 4;
            
            fill(#FFFFFF);
            textSize(200);  //文字の大きさ
            noStroke();
            textAlign(CENTER, CENTER);
            text("▼", 900, 300);
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
        else if (key == ' ')
        {
            mode = 9;
            
            image(bulb, 900, 300);
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
    textSize(50);  //文字の大きさ
    noStroke();
    textAlign(CENTER, CENTER);
    text(temp, 600, 100);
    text("°C", 650, 100);
    textSize(20);  //文字の大きさ
    textAlign(LEFT, CENTER);
    text("O -> Open", 565, 180);
    text("P -> Close", 565, 200);
    text("U -> Up", 565, 220);
    text("I -> Down", 565, 240);
    text("W -> Forward", 565, 260);
    text("A -> Left", 565, 280);
    text("S -> Back", 565, 300);
    text("D -> Right", 565, 320);
    text("SPACE -> Light", 565, 340);
    
    myPort.write((char)mode); //Portに信号を伝える
}

void serialEvent(Serial port)
{
  if (port.available() > 0)
  {
    temp = port.read();
  }
}
