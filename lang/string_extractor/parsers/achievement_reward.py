from ..write_text import write_text


def parse_achievement_reward(json, origin):
    write_text(json.get("category"), origin,
               comment="Achievement reward category")
    for choice in json.get("choices", []):
        write_text(choice.get("name"), origin,
                   comment="Achievement reward choice")
