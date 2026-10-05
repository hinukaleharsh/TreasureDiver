import os

import streamlit as st


GAME_URL = os.getenv(
    "TREASURE_DIVER_GAME_URL",
    "https://hinukaleharsh.github.io/Treasure-Diver/",
)

st.set_page_config(page_title="Treasure Diver", page_icon="🌊", layout="wide")

st.title("🌊 Treasure Diver")
st.write(
    "Dive for treasure, dodge the dangers below, and make it back to the boat "
    "before your oxygen runs out."
)

st.components.v1.iframe(GAME_URL, height=700, scrolling=False)

with st.expander("How to play"):
    st.markdown(
        """
        - Click the game and use **arrow keys** or **W A S D** to swim.
        - Hold **Left Shift** to boost; boosting uses oxygen faster.
        - Press **Space** to fire a harpoon at sharks.
        - Return to the surface to sell your treasure. Loot is lost if you die.
        - At the shop, press **1–5** to buy upgrades and **Enter** to dive.

        Your progress is saved in this browser.
        """
    )
