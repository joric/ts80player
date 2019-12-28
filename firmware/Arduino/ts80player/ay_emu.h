struct toneStruct {
  int count;
  int state;
};

struct noiseStruct {
  int count;
  int reg;
  int qcc;
  int state;
};

struct envStruct {
  int count;
  int dac;
  int up;
};

struct AYChipStruct {
  toneStruct tone[3];
  noiseStruct noise;
  envStruct env;

  int reg[16];
  int dac[3];
  int out[3];

  int freqDiv;
};



//�������� ��������� (��������, (�)HackerKAY)

#define VDIV	3

int volTab[16] = {
  0 / VDIV,
  836 / VDIV,
  1212 / VDIV,
  1773 / VDIV,
  2619 / VDIV,
  3875 / VDIV,
  5397 / VDIV,
  8823 / VDIV,
  10392 / VDIV,
  16706 / VDIV,
  23339 / VDIV,
  29292 / VDIV,
  36969 / VDIV,
  46421 / VDIV,
  55195 / VDIV,
  65535 / VDIV
};



void ay_init(AYChipStruct *ay)
{
  memset(ay, 0, sizeof(AYChipStruct));

  ay->noise.reg = 0x0ffff;
  ay->noise.qcc = 0;
  ay->noise.state = 0;
}



void ay_out(AYChipStruct *ay, int reg, int value)
{
  if (reg > 13) return;

  //������ � ������ �� ��������� ����� ��������� ������ ��� ����
  //�����, ��� ������ R13 ���������� ����� �������� ���������
  switch (reg)
  {
    case 1:
    case 3:
    case 5:
      value &= 15;
      break;
    case 8:
    case 9:
    case 10:
    case 6:
      value &= 31;
      break;
    case 13:
      value &= 15;
      ay->env.count = 0;
      if (value & 2)
      {
        ay->env.dac = 0;
        ay->env.up = 1;
      }
      else
      {
        ay->env.dac = 15;
        ay->env.up = 0;
      }
      break;
  }

  ay->reg[reg] = value;
}



inline void ay_tick(AYChipStruct *ay, int ticks)
{
  int noise_di;
  int i, ta, tb, tc, na, nb, nc;

  ay->out[0] = 0;
  ay->out[1] = 0;
  ay->out[2] = 0;

  for (i = 0; i < ticks; ++i)
  {
    //�������� �������� �������
    ay->freqDiv ^= 1;

    //����������

    if (ay->tone[0].count >= (ay->reg[0] | (ay->reg[1] << 8)))
    {
      ay->tone[0].count = 0;
      ay->tone[0].state ^= 1;
    }
    if (ay->tone[1].count >= (ay->reg[2] | (ay->reg[3] << 8)))
    {
      ay->tone[1].count = 0;
      ay->tone[1].state ^= 1;
    }
    if (ay->tone[2].count >= (ay->reg[4] | (ay->reg[5] << 8)))
    {
      ay->tone[2].count = 0;
      ay->tone[2].state ^= 1;
    }

    ay->tone[0].count++;
    ay->tone[1].count++;
    ay->tone[2].count++;


    if (ay->freqDiv)
    {

      //��� (�������� ��������, (C)HackerKAY)

      if (ay->noise.count == 0)
      {
        noise_di = (ay->noise.qcc ^ ((ay->noise.reg >> 13) & 1)) ^ 1;
        ay->noise.qcc = (ay->noise.reg >> 15) & 1;
        ay->noise.state = ay->noise.qcc;
        ay->noise.reg = (ay->noise.reg << 1) | noise_di;
      }

      ay->noise.count = (ay->noise.count + 1) & 31;
      if (ay->noise.count >= ay->reg[6]) ay->noise.count = 0;


      //���������

      if (ay->env.count == 0)
      {
        switch (ay->reg[13])
        {
          case 0:
          case 1:
          case 2:
          case 3:
          case 9:
            if (ay->env.dac > 0) ay->env.dac--;
            break;
          case 4:
          case 5:
          case 6:
          case 7:
          case 15:
            if (ay->env.up)
            {
              ay->env.dac++;
              if (ay->env.dac > 15)
              {
                ay->env.dac = 0;
                ay->env.up = 0;
              }
            }
            break;

          case 8:
            ay->env.dac--;
            if (ay->env.dac < 0) ay->env.dac = 15;
            break;

          case 10:
          case 14:
            if (!ay->env.up)
            {
              ay->env.dac--;
              if (ay->env.dac < 0)
              {
                ay->env.dac = 0;
                ay->env.up = 1;
              }
            }
            else
            {
              ay->env.dac++;
              if (ay->env.dac > 15)
              {
                ay->env.dac = 15;
                ay->env.up = 0;
              }

            }
            break;

          case 11:
            if (!ay->env.up)
            {
              ay->env.dac--;
              if (ay->env.dac < 0)
              {
                ay->env.dac = 15;
                ay->env.up = 1;
              }
            }
            break;

          case 12:
            ay->env.dac++;
            if (ay->env.dac > 15) ay->env.dac = 0;
            break;

          case 13:
            if (ay->env.dac < 15) ay->env.dac++;
            break;

        }
      }

      ay->env.count++;
      if (ay->env.count >= (ay->reg[11] | (ay->reg[12] << 8))) ay->env.count = 0;

    }

    //������

    ta = ay->tone[0].state | ((ay->reg[7] >> 0) & 1);
    tb = ay->tone[1].state | ((ay->reg[7] >> 1) & 1);
    tc = ay->tone[2].state | ((ay->reg[7] >> 2) & 1);
    na = ay->noise.state | ((ay->reg[7] >> 3) & 1);
    nb = ay->noise.state | ((ay->reg[7] >> 4) & 1);
    nc = ay->noise.state | ((ay->reg[7] >> 5) & 1);

    if (ay->reg[8] & 16)
    {
      ay->dac[0] = ay->env.dac;
    }
    else
    {
      if (ta & na) ay->dac[0] = ay->reg[8]; else ay->dac[0] = 0;
    }

    if (ay->reg[9] & 16)
    {
      ay->dac[1] = ay->env.dac;
    }
    else
    {
      if (tb & nb) ay->dac[1] = ay->reg[9]; else ay->dac[1] = 0;
    }

    if (ay->reg[10] & 16)
    {
      ay->dac[2] = ay->env.dac;
    }
    else
    {
      if (tc & nc) ay->dac[2] = ay->reg[10]; else ay->dac[2] = 0;
    }

    ay->out[0] += volTab[ay->dac[0]];
    ay->out[1] += volTab[ay->dac[1]];
    ay->out[2] += volTab[ay->dac[2]];
  }

  ay->out[0] /= ticks;
  ay->out[1] /= ticks;
  ay->out[2] /= ticks;
}
