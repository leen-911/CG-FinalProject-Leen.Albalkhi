  🌟 3D Lane Runner — Crazy Collector 101


A fast‑paced 3D lane‑runner game built using C++, OpenGL 3.3, GLFW, GLEW, GLM, and stb_image.
The player moves between lanes, jumps over obstacles, collects coins, and races against a 60‑second timer.


    🎮 Game Overview
Crazy Collector 101 is a 3D endless‑runner style game where the player controls a rolling ball moving forward on a long road.
Your goal is simple:

🪙Collect as many coins as possible

🧱Avoid obstacles

⌛Survive until the timer reaches zero

🔁Or reset and try again

    The game features:

-Three‑lane movement

-Jumping mechanics

-Hundreds of coins

-Dozens of obstacles

-Dynamic lighting

-Scrolling textures

-Skybox environment

-Smooth camera following

-Win/Lose states

-Real‑time score and timer display


    ⌨️ Controls

  
⬅️| **LEFT Arrow** | Move to the left lane |

➡️| **RIGHT Arrow** | Move to the right lane |

⬆️| **UP Arrow** | Jump |

| **R** | Reset the game |

| **ESC** | Quit the game |


    🛠️ Build Instructions
Requirements

This project uses:

-OpenGL 3.3 Core

-GLFW (window + input)

-GLEW (OpenGL loader)

-GLM (math)

-stb_image (texture loading)

-All required headers are included in the project folder.





    ▶️ Running the Game
When the game starts:

-The ball begins rolling forward automatically

-The timer starts counting down from 60 seconds

-Coins and obstacles spawn along the road

-The camera follows the player smoothly

You lose if:

-You hit an obstacle

-The timer reaches zero

-You can always press R to restart instantly.


    🧠 How the Game Works (Technical Summary)
-The ball moves forward automatically using ballZ -= speed * deltaTime

-Lane switching updates the X‑position instantly

-Jumping uses velocity + gravity physics

-Coins and obstacles are stored in arrays and checked using distance formulas

-Shaders handle:

-Lighting

-Texture scrolling

-Skybox rendering

-The camera uses glm::lookAt to follow the player smoothly

-The window title updates every frame with score + time


## 🎥 Demo Video
[Click here to watch the gameplay demo](https://github.com/user-attachments/assets/59a65906-8408-44b3-b5c6-c6bd87f62708)

