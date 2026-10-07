-- HexEmpire: optional private cloud saves. Gameplay remains offline/client-side.
begin;

create table public.profiles (
  id uuid primary key references auth.users(id) on delete cascade,
  display_name text not null default 'Commander'
    check (char_length(display_name) between 1 and 40),
  created_at timestamptz not null default now()
);

create table public.game_saves (
  id uuid primary key default gen_random_uuid(),
  owner_id uuid not null references auth.users(id) on delete cascade,
  slot text not null default 'autosave'
    check (slot ~ '^[a-zA-Z0-9_-]{1,32}$'),
  schema_version integer not null default 1 check (schema_version = 1),
  state jsonb not null,
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now(),
  unique (owner_id, slot),
  constraint save_object check (jsonb_typeof(state) = 'object'),
  constraint save_size check (octet_length(state::text) <= 1048576),
  constraint save_keys check (
    state ?& array['version','map_size','map_seed','round','turn_index',
      'next_uid','finished','winner_id','tiles','units','factions','fog']
    and state->'version' = '1'::jsonb
    and state->'map_size' in ('10'::jsonb, '16'::jsonb, '24'::jsonb)
    and jsonb_typeof(state->'tiles') = 'array'
    and jsonb_typeof(state->'units') = 'array'
    and jsonb_typeof(state->'factions') = 'array'
    and jsonb_typeof(state->'fog') = 'object'
  )
);

create function public.set_save_updated_at()
returns trigger language plpgsql set search_path = '' as $$
begin
  new.created_at := old.created_at;
  new.updated_at := now();
  return new;
end;
$$;

create trigger saves_updated_at before update on public.game_saves
for each row execute function public.set_save_updated_at();

create function public.create_player_profile()
returns trigger language plpgsql security definer set search_path = '' as $$
begin
  insert into public.profiles(id) values(new.id) on conflict (id) do nothing;
  return new;
end;
$$;

create trigger on_hexempire_user_created after insert on auth.users
for each row execute function public.create_player_profile();

-- Backfill users already registered before this migration.
insert into public.profiles(id) select id from auth.users on conflict (id) do nothing;

alter table public.profiles enable row level security;
alter table public.game_saves enable row level security;

revoke all on public.profiles, public.game_saves from anon, authenticated;
grant select on public.profiles to authenticated;
grant update (display_name) on public.profiles to authenticated;
grant select, insert, update, delete on public.game_saves to authenticated;

create policy profiles_read_self on public.profiles for select to authenticated
using ((select auth.uid()) = id);
create policy profiles_update_self on public.profiles for update to authenticated
using ((select auth.uid()) = id) with check ((select auth.uid()) = id);

create policy saves_read_self on public.game_saves for select to authenticated
using ((select auth.uid()) = owner_id);
create policy saves_insert_self on public.game_saves for insert to authenticated
with check ((select auth.uid()) = owner_id);
create policy saves_update_self on public.game_saves for update to authenticated
using ((select auth.uid()) = owner_id) with check ((select auth.uid()) = owner_id);
create policy saves_delete_self on public.game_saves for delete to authenticated
using ((select auth.uid()) = owner_id);

revoke all on function public.create_player_profile() from public, anon, authenticated;
revoke all on function public.set_save_updated_at() from public, anon, authenticated;
commit;
