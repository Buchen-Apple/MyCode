using System;
using System.Threading;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ConsoleApp1
{
    // 동물들 베이스
    class MammalBase
    {
        // 0 : 개 / 1 : 고양이
        int iType;

        public MammalBase(int iType)
        {
            this.iType = iType;
        }

        public virtual void howl(int Input)
        { }
    }

    class Dog : MammalBase
    {
        public Dog(int type) : base(type)
        {}

        public override void howl(int Input)
        {
            int randVal;

            unsafe
            {
                Random rand = new Random();
                randVal = rand.Next(0, 1574) + (int)&randVal;

                randVal = (randVal * (Input + 1)) % 3;               
            }

            // 유저 액션과 완전히 동일하면 기분 좋음.
            // 다르면, 상황에 따라 다른 반응
            if (Input == 0)
            {
                switch (randVal)
                {
                    case 0:
                        Console.Write("개 : 기분 매우매우매우 좋다멍!");
                        break;
                    case 1:
                        Console.Write("개 : 좋다멍!");
                        break;
                    case 2:
                        Console.Write("개 : 완전 별로다멍...");
                        break;
                    default:
                        break;
                }
            }

            else if (Input == 1)
            {
                switch (randVal)
                {
                    case 0:
                        Console.Write("개 : 완전 별로다멍...");
                        break;
                    case 1:
                        Console.Write("개 : 기분 매우매우매우 좋다멍!");
                        break;
                    case 2:
                        Console.Write("개 : 좋다멍!");                       
                        break;

                    default:
                        break;
                }

            }

            else
            {
                switch (randVal)
                {
                    case 0:
                        Console.Write("개 : 완전 별로다멍...");
                        break;
                    case 1:
                        Console.Write("개 : 좋다멍!");
                        break;
                    case 2:                       
                        Console.Write("개 : 기분 매우매우매우 좋다멍!");
                        break;
                    default:
                        break;
                }

            }

           
        }
    }

    class Cat : MammalBase
    {
        public Cat(int type) : base(type)
        { }

        public override void howl(int Input)
        {
            int randVal;

            unsafe
            {
                Random rand = new Random();
                randVal = rand.Next(0, 12575) + (int)&randVal;

                randVal = (randVal * (Input + 1)) % 3;
            }

            if (Input == 0)
            {
                switch (randVal)
                {
                    case 0:
                        Console.Write("고양이 : 기분 매우매우매우 좋다냥!");
                        break;
                    case 1:
                        Console.Write("고양이 : 좋다냥!");
                        break;
                    case 2:
                        Console.Write("고양이 : 완전 별루다냥...!");
                        break;
                }
            }

            else if (Input == 1)
            {
                switch (randVal)
                {
                    case 0:
                        Console.Write("고양이 : 완전 별루다냥...!");
                        break;
                    case 1:
                        Console.Write("고양이 : 기분 매우매우매우 좋다냥!");
                        break;
                    case 2:
                        Console.Write("고양이 : 좋다냥!");                       
                        break;
                }
            }

            else
            {
                switch (randVal)
                {
                    case 0:
                        Console.Write("고양이 : 완전 별루다냥...!");                        
                        break;
                    case 1:
                        Console.Write("고양이 : 좋다냥!");
                        break;
                    case 2:
                        Console.Write("고양이 : 기분 매우매우매우 좋다냥!");
                        break;
                }
            }
        }
    }

    class User
    {
        // 유저의 행동. 누군가 씻기기
        public void Wash(ref MammalBase Nobody, int Input)
        {
            Console.Write("동물의 반응 >>> [");
            Nobody.howl(Input);
            Console.WriteLine("]");
        }
    }


    class MAIN
    {
        static void Main(string[] args)
        {
            // 개, 고양이 생성
            MammalBase Dog = new Dog(0);
            MammalBase Cat = new Cat(0);

            // 유저 생성
            User Player = new User();

            while (true)
            {
                // 액션 선택
                Console.Write("동물을 관리합니다 (1 : 털 손질 / 2 : 목욕 / 3 : 밥주기) : ");
                string str = Console.ReadLine();
                int Input = int.Parse(str);

                Input--;

                // 개 씻기기
                Player.Wash(ref Dog, Input);

                // 고양이 씻기기
                Player.Wash(ref Cat, Input);

                Console.WriteLine();             
            }

        }
    }
}
